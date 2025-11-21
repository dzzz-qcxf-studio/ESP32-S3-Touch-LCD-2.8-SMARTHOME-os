# 项目介绍

基于斑梨电子的ESP32-S3-Touch-LCD-2.8开发板的智能物联架构，融入ai云控的。ESP负责HMI交互与后端ai链接，STM32负责执行各个任务。

## 使用硬件

HMI交互设备：斑梨电子的ESP32-S3-Touch-LCD-2.8开发板
执行设备：正点原子stm32f103zet6开发板

## 帮助

1：使用esp-idf进行开发。
2：如果无法编译请检查cmake文件或者路径是否正确，请务必保证不要有中文。
3：如果增改字库，使用url:https://lvgl.io/tools/fontconverter 。
4：新的myfont.c不能直接替换，替换
```
/*--------------------
 *  ALL CUSTOM DATA
 *--------------------*/
```
以上部分即可。
