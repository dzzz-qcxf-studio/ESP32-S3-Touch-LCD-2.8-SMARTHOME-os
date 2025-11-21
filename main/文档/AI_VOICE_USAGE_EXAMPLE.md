# AI语音界面使用示例

## 概述
本文档展示如何正确使用AI语音界面的回调机制，确保资源正确管理。

---

## 完整示例代码

```c
#include "ai_chat_ui.h"

// 全局变量示例（根据实际需求定义）
static bool is_recording = false;
static bool is_playing_tts = false;

/**
 * 语音开始回调 - 用户点击麦克风开始说话
 */
void on_voice_start(void) 
{
    printf("[AI] 开始录音\n");
    
    // TODO: 启动麦克风录音
    // - 初始化I2S/麦克风驱动
    // - 开始录音到缓冲区
    // - 设置录音标志
    is_recording = true;
}

/**
 * 语音停止回调 - 用户再次点击麦克风停止
 */
void on_voice_stop(void) 
{
    printf("[AI] 停止录音，开始处理\n");
    
    // TODO: 停止录音并发送到AI服务器
    // - 停止麦克风录音
    // - 保存音频数据
    // - 发送到AI识别服务器
    is_recording = false;
    
    // 模拟识别过程
    // 识别完成后显示用户说的话
    ai_chat_ui_add_message(1, "打开客厅的灯");
    
    // 模拟AI处理和TTS播放
    ai_chat_ui_set_voice_state(AI_VOICE_SPEAKING);
    ai_chat_ui_add_message(0, "好的，已为您打开客厅的灯");
    is_playing_tts = true;
    
    // TODO: 播放TTS音频
    // 播放完成后调用: on_tts_finished();
}

/**
 * 资源清理回调 - 用户中途退出界面
 * 这是关键：确保退出时所有资源被释放
 */
void on_voice_cleanup(void) 
{
    printf("[AI] 清理语音资源\n");
    
    // TODO: 强制停止所有语音相关任务
    
    // 1. 如果正在录音，立即停止
    if (is_recording) {
        // 停止麦克风
        // 释放录音缓冲区
        is_recording = false;
        printf("[AI] 停止录音\n");
    }
    
    // 2. 如果正在播放TTS，立即停止
    if (is_playing_tts) {
        // 停止TTS播放
        // 释放音频资源
        is_playing_tts = false;
        printf("[AI] 停止TTS播放\n");
    }
    
    // 3. 取消网络请求（如果有）
    // cancel_ai_request();
    
    // 4. 释放其他资源
    printf("[AI] 所有资源已清理\n");
}

/**
 * TTS播放完成回调（由TTS驱动调用）
 */
void on_tts_finished(void)
{
    is_playing_tts = false;
    ai_chat_ui_set_voice_state(AI_VOICE_IDLE);
    printf("[AI] TTS播放完成，返回空闲状态\n");
}

/**
 * 初始化AI语音界面
 */
void init_ai_voice_ui(void)
{
    // 注册所有回调
    ai_chat_ui_register_voice_start_callback(on_voice_start);
    ai_chat_ui_register_voice_stop_callback(on_voice_stop);
    ai_chat_ui_register_voice_cleanup_callback(on_voice_cleanup);  // 重要！
    
    printf("[AI] 语音界面回调已注册\n");
}
```

---

## 关键改进说明

### 1. **新增清理回调**
```c
void ai_chat_ui_register_voice_cleanup_callback(ai_voice_cleanup_callback_t callback);
```
- **何时触发**：用户点击返回按钮或调用 `ai_chat_ui_destroy()` 时
- **作用**：强制停止所有正在进行的语音任务，释放资源
- **必须实现**：确保无论在哪个状态退出都能正确清理

### 2. **自动状态重置**
- 每次调用 `ai_chat_ui_create()` 时自动重置状态为 `AI_VOICE_IDLE`
- 防止上次未正常退出导致的状态残留

### 3. **内部清理机制**
```c
static void ai_chat_ui_cleanup(void)
{
    /* 如果正在监听或处理，调用清理回调 */
    if (voice_cleanup_callback && current_voice_state != AI_VOICE_IDLE) {
        voice_cleanup_callback();
    }
    
    /* 重置所有状态 */
    current_voice_state = AI_VOICE_IDLE;
}
```

---

## 使用场景

### 场景1：正常流程
1. 用户点击麦克风 → `on_voice_start()` 被调用
2. 用户再次点击 → `on_voice_stop()` 被调用
3. AI处理完成 → 设置状态为 `AI_VOICE_IDLE`
4. 用户点击返回 → `on_voice_cleanup()` 被调用（但因为状态是IDLE，不需要额外清理）

### 场景2：录音中途退出（你的问题场景）
1. 用户点击麦克风 → `on_voice_start()` 被调用，开始录音
2. **用户点击返回按钮**
3. → `on_voice_cleanup()` 被调用
4. → 停止录音，释放资源
5. → 状态重置为 `AI_VOICE_IDLE`
6. 用户再次进入界面 → 状态正常，按钮可用 ✅

### 场景3：AI处理中退出
1. 用户说完话，AI正在处理（状态：`AI_VOICE_PROCESSING`）
2. **用户点击返回按钮**
3. → `on_voice_cleanup()` 被调用
4. → 取消网络请求，释放资源
5. → 状态重置
6. 再次进入 → 正常 ✅

### 场景4：TTS播放中退出
1. AI回复播放中（状态：`AI_VOICE_SPEAKING`）
2. **用户点击返回按钮**
3. → `on_voice_cleanup()` 被调用
4. → 停止TTS播放
5. → 状态重置
6. 再次进入 → 正常 ✅

---

## 调试技巧

```c
void on_voice_cleanup(void) 
{
    ai_voice_state_t state = ai_chat_ui_get_voice_state();
    printf("[AI] 清理时状态：%d\n", state);
    
    switch(state) {
        case AI_VOICE_LISTENING:
            printf("[AI] 正在录音 → 停止\n");
            break;
        case AI_VOICE_PROCESSING:
            printf("[AI] 正在处理 → 取消\n");
            break;
        case AI_VOICE_SPEAKING:
            printf("[AI] 正在播放 → 停止\n");
            break;
        default:
            printf("[AI] 空闲状态，无需清理\n");
            break;
    }
}
```

---

## 总结

✅ **问题已解决**：
- 退出界面时自动调用清理回调
- 状态正确重置为 `AI_VOICE_IDLE`
- 再次进入时按钮完全可用

✅ **最佳实践**：
- 务必注册 `cleanup_callback`
- 在清理回调中释放所有资源
- 使用状态判断需要清理的资源类型

✅ **扩展性**：
- 可以在清理回调中添加更多资源管理逻辑
- 支持多种语音任务同时管理
