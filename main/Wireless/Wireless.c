#include "Wireless.h"
#include <ctype.h>

#define WIFI_PROV_NAMESPACE      "wifi_cfg"
#define WIFI_PROV_KEY_SSID       "ssid"
#define WIFI_PROV_KEY_PASS       "pass"
#define WIFI_PROV_AP_SSID        "SmartHome-Setup"
#define WIFI_PROV_AP_PASS        "12345678"
#define WIFI_PROV_AP_CHANNEL     1
#define WIFI_MAXIMUM_RETRY       5

uint16_t BLE_NUM = 0;
uint16_t WIFI_NUM = 0;
bool Scan_finish = 0;

bool WiFi_Scan_Finish = 0;
bool BLE_Scan_Finish = 0;

static const char *TAG_WIFI = "Wireless";

static bool s_wifi_provisioned = false;
static esp_netif_t *s_netif_sta = NULL;
static esp_netif_t *s_netif_ap = NULL;
static httpd_handle_t s_http_server = NULL;
static char s_wifi_ssid[33] = {0};
static char s_wifi_pass[65] = {0};                   
static int s_retry_num = 0;

static void wifi_start_softap(void);
static void wifi_start_sta(const char *ssid, const char *password);

static bool wifi_credentials_load(char *ssid, size_t ssid_len, char *password, size_t pass_len);
static esp_err_t wifi_credentials_save(const char *ssid, const char *password);
static void url_decode(char *dst, const char *src, size_t len);
static esp_err_t provisioning_http_start(void);
static void provisioning_http_stop(void);
static esp_err_t provision_root_get_handler(httpd_req_t *req);
static esp_err_t provision_config_get_handler(httpd_req_t *req);
static void wifi_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data);
static void ip_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data);

void Wireless_Init(void)
{
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    xTaskCreatePinnedToCore(
        WIFI_Init,
        "WIFI task",
        4096,
        NULL,
        3,
        NULL,
        0);
    xTaskCreatePinnedToCore(
        BLE_Init, 
        "BLE task",
        4096, 
        NULL, 
        2, 
        NULL, 
        0);
}

void WIFI_Init(void *arg)
{
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    if (!s_netif_sta) {
        s_netif_sta = esp_netif_create_default_wifi_sta();
    }
    if (!s_netif_ap) {
        s_netif_ap = esp_netif_create_default_wifi_ap();
    }

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &ip_event_handler, NULL));

    s_wifi_provisioned = wifi_credentials_load(s_wifi_ssid, sizeof(s_wifi_ssid), s_wifi_pass, sizeof(s_wifi_pass));
    ESP_LOGI(TAG_WIFI, "Provisioned: %s", s_wifi_provisioned ? "yes" : "no");

    if (s_wifi_provisioned) {
        wifi_start_sta(s_wifi_ssid, s_wifi_pass);
    } else {
        wifi_start_softap();
    }

    WIFI_NUM = WIFI_Scan();
    printf("WIFI:%d\r\n",WIFI_NUM);
    
    vTaskDelete(NULL);
}

uint16_t WIFI_Scan(void)
{
    wifi_mode_t mode;
    if (esp_wifi_get_mode(&mode) != ESP_OK || (mode != WIFI_MODE_STA && mode != WIFI_MODE_APSTA)) {
        return 0;
    }

    uint16_t ap_count = 0;
    ESP_ERROR_CHECK(esp_wifi_scan_start(NULL, true));
    ESP_ERROR_CHECK(esp_wifi_scan_get_ap_num(&ap_count));
    ESP_ERROR_CHECK(esp_wifi_scan_stop());
    WiFi_Scan_Finish = 1;
    if (BLE_Scan_Finish == 1) {
        Scan_finish = 1;
    }
    return ap_count;
}

bool Wireless_Is_Provisioned(void)
{
    return s_wifi_provisioned;
}

static void wifi_start_softap(void)
{
    ESP_LOGI(TAG_WIFI, "Starting SoftAP provisioning");
    wifi_config_t ap_config = {0};
    strlcpy((char *)ap_config.ap.ssid, WIFI_PROV_AP_SSID, sizeof(ap_config.ap.ssid));
    ap_config.ap.ssid_len = strlen(WIFI_PROV_AP_SSID);
    ap_config.ap.channel = WIFI_PROV_AP_CHANNEL;
    ap_config.ap.max_connection = 4;
    strlcpy((char *)ap_config.ap.password, WIFI_PROV_AP_PASS, sizeof(ap_config.ap.password));
    if (strlen(WIFI_PROV_AP_PASS) == 0) {
        ap_config.ap.authmode = WIFI_AUTH_OPEN;
    } else {
        ap_config.ap.authmode = WIFI_AUTH_WPA_WPA2_PSK;
    }

    provisioning_http_start();
    ESP_ERROR_CHECK(esp_wifi_stop());
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &ap_config));
    ESP_ERROR_CHECK(esp_wifi_start());
}

static void wifi_start_sta(const char *ssid, const char *password)
{
    ESP_LOGI(TAG_WIFI, "Starting STA connection to %s", ssid);
    provisioning_http_stop();
    wifi_config_t sta_config = {0};
    strlcpy((char *)sta_config.sta.ssid, ssid, sizeof(sta_config.sta.ssid));
    strlcpy((char *)sta_config.sta.password, password, sizeof(sta_config.sta.password));
    sta_config.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;
    sta_config.sta.sae_pwe_h2e = WPA3_SAE_PWE_BOTH;

    ESP_ERROR_CHECK(esp_wifi_stop());
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &sta_config));
    ESP_ERROR_CHECK(esp_wifi_start());
    ESP_ERROR_CHECK(esp_wifi_connect());
}

static bool wifi_credentials_load(char *ssid, size_t ssid_len, char *password, size_t pass_len)
{
    nvs_handle_t handle;
    if (nvs_open(WIFI_PROV_NAMESPACE, NVS_READONLY, &handle) != ESP_OK) {
        return false;
    }

    size_t len = ssid_len;
    esp_err_t err = nvs_get_str(handle, WIFI_PROV_KEY_SSID, ssid, &len);
    if (err != ESP_OK) {
        nvs_close(handle);
        return false;
    }

    len = pass_len;
    err = nvs_get_str(handle, WIFI_PROV_KEY_PASS, password, &len);
    nvs_close(handle);
    if (err != ESP_OK) {
        return false;
    }
    return strlen(ssid) > 0;
}

static esp_err_t wifi_credentials_save(const char *ssid, const char *password)
{
    nvs_handle_t handle;
    ESP_ERROR_CHECK(nvs_open(WIFI_PROV_NAMESPACE, NVS_READWRITE, &handle));
    ESP_ERROR_CHECK(nvs_set_str(handle, WIFI_PROV_KEY_SSID, ssid));
    ESP_ERROR_CHECK(nvs_set_str(handle, WIFI_PROV_KEY_PASS, password));
    esp_err_t err = nvs_commit(handle);
    nvs_close(handle);
    return err;
}

static void url_decode(char *dst, const char *src, size_t len)
{
    size_t i = 0, j = 0;
    while (i < len && src[i] != '\0') {
        if (src[i] == '%' && i + 2 < len) {
            char hex[3] = {src[i + 1], src[i + 2], '\0'};
            dst[j++] = (char)strtol(hex, NULL, 16);
            i += 3;
        } else if (src[i] == '+') {
            dst[j++] = ' ';
            i++;
        } else {
            dst[j++] = src[i++];
        }
    }
    dst[j] = '\0';
}

static esp_err_t provision_root_get_handler(httpd_req_t *req)
{
    static const char *page =
        "<html><head><title>Wi-Fi Setup</title></head><body>"
        "<h2>ESP32 智能家居配网</h2>"
        "<form method=\"POST\" action=\"/config\">"
        "SSID:<br><input name=\"ssid\" maxlength=32><br>"
        "Password:<br><input name=\"password\" type=\"password\" maxlength=64><br><br>"
        "<input type=\"submit\" value=\"连接\">"
        "</form></body></html>";
    return httpd_resp_send(req, page, HTTPD_RESP_USE_STRLEN);
}

static esp_err_t provision_config_get_handler(httpd_req_t *req)
{
    int total_len = req->content_len;
    if (total_len <= 0) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Empty body");
        return ESP_FAIL;
    }

    char buf[160] = {0};
    int received = 0;
    while (received < total_len) {
        int cur = httpd_req_recv(req, buf + received, sizeof(buf) - 1 - received);
        if (cur <= 0) {
            return ESP_FAIL;
        }
        received += cur;
        if (received >= sizeof(buf) - 1) {
            break;
        }
    }
    buf[received] = '\0';

    char *ssid_start = strstr(buf, "ssid=");
    char *pass_start = strstr(buf, "password=");
    if (!ssid_start || !pass_start) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Invalid form");
        return ESP_FAIL;
    }

    char ssid_enc[64] = {0};
    char pass_enc[96] = {0};

    sscanf(ssid_start, "ssid=%63[^&]", ssid_enc);
    sscanf(pass_start, "password=%95[^&]", pass_enc);

    char ssid[33] = {0};
    char password[65] = {0};
    url_decode(ssid, ssid_enc, sizeof(ssid_enc));
    url_decode(password, pass_enc, sizeof(pass_enc));

    if (strlen(ssid) == 0) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "SSID required");
        return ESP_FAIL;
    }

    wifi_credentials_save(ssid, password);
    s_wifi_provisioned = true;
    httpd_resp_sendstr(req, "配置已保存, 正在连接...");

    wifi_start_sta(ssid, password);
    return ESP_OK;
}

static esp_err_t provisioning_http_start(void)
{
    if (s_http_server) {
        return ESP_OK;
    }

    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.server_port = 80;

    esp_err_t err = httpd_start(&s_http_server, &config);
    if (err != ESP_OK) {
        ESP_LOGE(TAG_WIFI, "Failed to start HTTP server: %s", esp_err_to_name(err));
        return err;
    }

    httpd_uri_t root = {
        .uri = "/",
        .method = HTTP_GET,
        .handler = provision_root_get_handler,
        .user_ctx = NULL,
    };
    httpd_register_uri_handler(s_http_server, &root);

    httpd_uri_t config_uri = {
        .uri = "/config",
        .method = HTTP_POST,
        .handler = provision_config_get_handler,
        .user_ctx = NULL,
    };
    httpd_register_uri_handler(s_http_server, &config_uri);

    ESP_LOGI(TAG_WIFI, "Provisioning HTTP server started");
    return ESP_OK;
}

static void provisioning_http_stop(void)
{
    if (s_http_server) {
        httpd_stop(s_http_server);
        s_http_server = NULL;
        ESP_LOGI(TAG_WIFI, "Provisioning HTTP server stopped");
    }
}

static void wifi_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    if (event_base == WIFI_EVENT) {
        switch (event_id) {
        case WIFI_EVENT_STA_START:
            esp_wifi_connect();
            break;
        case WIFI_EVENT_STA_DISCONNECTED:
            if (s_retry_num < WIFI_MAXIMUM_RETRY) {
                esp_wifi_connect();
                s_retry_num++;
                ESP_LOGW(TAG_WIFI, "Retrying connection (%d)", s_retry_num);
            } else {
                ESP_LOGW(TAG_WIFI, "Failed to connect, switching to SoftAP");
                s_retry_num = 0;
                s_wifi_provisioned = false;
                wifi_start_softap();
            }
            break;
        case WIFI_EVENT_AP_START:
            provisioning_http_start();
            break;
        default:
            break;
        }
    }
}

static void ip_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
        ESP_LOGI(TAG_WIFI, "Got IP: " IPSTR, IP2STR(&event->ip_info.ip));
        s_retry_num = 0;
        provisioning_http_stop();
    }
}

#define GATTC_TAG "GATTC_TAG"
#define SCAN_DURATION 5  
#define MAX_DISCOVERED_DEVICES 100 

typedef struct {
    uint8_t address[6];
    bool is_valid;
} discovered_device_t;

static discovered_device_t discovered_devices[MAX_DISCOVERED_DEVICES];
static size_t num_discovered_devices = 0;
static size_t num_devices_with_name = 0; 

static bool is_device_discovered(const uint8_t *addr) {
    for (size_t i = 0; i < num_discovered_devices; i++) {
        if (memcmp(discovered_devices[i].address, addr, 6) == 0) {
            return true;
        }
    }
    return false;
}

static void add_device_to_list(const uint8_t *addr) {
    if (num_discovered_devices < MAX_DISCOVERED_DEVICES) {
        memcpy(discovered_devices[num_discovered_devices].address, addr, 6);
        discovered_devices[num_discovered_devices].is_valid = true;
        num_discovered_devices++;
    }
}

static bool extract_device_name(const uint8_t *adv_data, uint8_t adv_data_len, char *device_name, size_t max_name_len) {
    size_t offset = 0;
    while (offset < adv_data_len) {
        if (adv_data[offset] == 0) break; 

        uint8_t length = adv_data[offset];
        if (length == 0 || offset + length > adv_data_len) break; 

        uint8_t type = adv_data[offset + 1];
        if (type == ESP_BLE_AD_TYPE_NAME_CMPL || type == ESP_BLE_AD_TYPE_NAME_SHORT) {
            if (length > 1 && length - 1 < max_name_len) {
                memcpy(device_name, &adv_data[offset + 2], length - 1);
                device_name[length - 1] = '\0'; 
                return true;
            } else {
                return false;
            }
        }
        offset += length + 1;
    }
    return false;
}

static void esp_gap_cb(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param) {
    static char device_name[100]; 

    switch (event) {
        case ESP_GAP_BLE_SCAN_RESULT_EVT:
            if (param->scan_rst.search_evt == ESP_GAP_SEARCH_INQ_RES_EVT) {
                if (!is_device_discovered(param->scan_rst.bda)) {
                    add_device_to_list(param->scan_rst.bda);
                    BLE_NUM++; 

                    if (extract_device_name(param->scan_rst.ble_adv, param->scan_rst.adv_data_len, device_name, sizeof(device_name))) {
                        num_devices_with_name++;
                        // printf("Found device: %02X:%02X:%02X:%02X:%02X:%02X\n        Name: %s\n        RSSI: %d\r\n",
                        //          param->scan_rst.bda[0], param->scan_rst.bda[1],
                        //          param->scan_rst.bda[2], param->scan_rst.bda[3],
                        //          param->scan_rst.bda[4], param->scan_rst.bda[5],
                        //          device_name, param->scan_rst.rssi);
                        // printf("\r\n");
                    } else {
                        // printf("Found device: %02X:%02X:%02X:%02X:%02X:%02X\n        Name: Unknown\n        RSSI: %d\r\n",
                        //          param->scan_rst.bda[0], param->scan_rst.bda[1],
                        //          param->scan_rst.bda[2], param->scan_rst.bda[3],
                        //          param->scan_rst.bda[4], param->scan_rst.bda[5],
                        //          param->scan_rst.rssi);
                        // printf("\r\n");
                    }
                }
            }
            break;
        case ESP_GAP_BLE_SCAN_STOP_COMPLETE_EVT:
            ESP_LOGI(GATTC_TAG, "Scan complete. Total devices found: %d (with names: %d)", BLE_NUM, num_devices_with_name);
            break;
        default:
            break;
    }
}

void BLE_Init(void *arg)
{
    ESP_ERROR_CHECK(esp_bt_controller_mem_release(ESP_BT_MODE_CLASSIC_BT));
    esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
    esp_err_t ret = esp_bt_controller_init(&bt_cfg);                                            
    if (ret) {
        printf("%s initialize controller failed: %s\n", __func__, esp_err_to_name(ret));        
        return;}
    ret = esp_bt_controller_enable(ESP_BT_MODE_BLE);                                            
    if (ret) {
        printf("%s enable controller failed: %s\n", __func__, esp_err_to_name(ret));            
        return;}
    ret = esp_bluedroid_init();                                                                 
    if (ret) {
        printf("%s init bluetooth failed: %s\n", __func__, esp_err_to_name(ret));               
        return;}
    ret = esp_bluedroid_enable();                                                               
    if (ret) {
        printf("%s enable bluetooth failed: %s\n", __func__, esp_err_to_name(ret));             
        return;}

    //register the  callback function to the gap module
    ret = esp_ble_gap_register_callback(esp_gap_cb);                                            
    if (ret){
        printf("%s gap register error, error code = %x\n", __func__, ret);                      
        return;
    }
    BLE_Scan();
    // while(1)
    // {
    //     vTaskDelay(pdMS_TO_TICKS(150));
    // }
    
    vTaskDelete(NULL);

}
uint16_t BLE_Scan(void)
{
    esp_ble_scan_params_t scan_params = {
        .scan_type = BLE_SCAN_TYPE_ACTIVE,
        .own_addr_type = BLE_ADDR_TYPE_RPA_PUBLIC,
        .scan_filter_policy = BLE_SCAN_FILTER_ALLOW_ALL,
        .scan_interval = 0x50,     
        .scan_window = 0x30,        
        .scan_duplicate         = BLE_SCAN_DUPLICATE_DISABLE
    };
    ESP_ERROR_CHECK(esp_ble_gap_set_scan_params(&scan_params));

    printf("Starting BLE scan...\n");
    ESP_ERROR_CHECK(esp_ble_gap_start_scanning(SCAN_DURATION));
    
    // Set scanning duration
    vTaskDelay(SCAN_DURATION * 1000 / portTICK_PERIOD_MS);
    
    printf("Stopping BLE scan...\n");
    // ESP_ERROR_CHECK(esp_ble_gap_stop_scanning());
    ESP_ERROR_CHECK(esp_ble_dtm_stop());
    BLE_Scan_Finish = 1;
    if(WiFi_Scan_Finish == 1)
        Scan_finish = 1;
    return BLE_NUM;
}