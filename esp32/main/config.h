#ifndef _BOARD_CONFIG_H_
#define _BOARD_CONFIG_H_

#include <driver/gpio.h>
#include <driver/uart.h>

//
// === 1. Audio I²S (INMP441 麦克风 + MAX98357 功放) ===
//
#define AUDIO_INPUT_SAMPLE_RATE 16000
#define AUDIO_OUTPUT_SAMPLE_RATE 24000

// 使用 I²S Simplex 模式：输入/输出分别独立引脚
#define AUDIO_I2S_METHOD_SIMPLEX

#ifdef AUDIO_I2S_METHOD_SIMPLEX
// INMP441 麦克风 (I²S 输入)
#define AUDIO_I2S_MIC_GPIO_WS GPIO_NUM_10  // LRCK / WS
#define AUDIO_I2S_MIC_GPIO_SCK GPIO_NUM_9  // BCLK
#define AUDIO_I2S_MIC_GPIO_DIN GPIO_NUM_38 // SD 数据输入

// MAX98357 功放 (I²S 输出)
#define AUDIO_I2S_SPK_GPIO_BCLK GPIO_NUM_11 // BCLK
#define AUDIO_I2S_SPK_GPIO_WS GPIO_NUM_12   // LRCK / WS
#define AUDIO_I2S_SPK_GPIO_DOUT GPIO_NUM_13 // DIN 数据输出
#else
// 全双工模式示例（如需）
#define AUDIO_I2S_GPIO_WS GPIO_NUM_10
#define AUDIO_I2S_GPIO_BCLK GPIO_NUM_9
#define AUDIO_I2S_GPIO_DIN GPIO_NUM_38
#define AUDIO_I2S_GPIO_DOUT GPIO_NUM_13
#endif

//
// === 2. WS2812B 灯带 ===
//
#define WS2812_LEFT_STRIP_GPIO GPIO_NUM_19
#define WS2812_RIGHT_STRIP_GPIO GPIO_NUM_21
#define WS2812_TOP_RING_GPIO GPIO_NUM_8

//
// === 3. 按钮 ===
//
#define BUTTON_GPIO GPIO_NUM_33 // 圆形按钮，低电平触发

//
// === 4. UART (ESP32-S3 ↔ STM32F103) ===
//
#define STM32_UART_PORT UART_NUM_1
#define STM32_UART_TX GPIO_NUM_17
#define STM32_UART_RX GPIO_NUM_18

//

//
// === 6. TFT 显示屏 (ST7796S, 480×320) ===
//
#define DISPLAY_WIDTH 480
#define DISPLAY_HEIGHT 320

#define DISPLAY_MOSI_PIN GPIO_NUM_42
#define DISPLAY_CLK_PIN GPIO_NUM_40
#define DISPLAY_CS_PIN GPIO_NUM_39
#define DISPLAY_DC_PIN GPIO_NUM_45
#define DISPLAY_RST_PIN GPIO_NUM_48
#define DISPLAY_BACKLIGHT_PIN GPIO_NUM_47

// 屏幕参数
#define DISPLAY_BACKLIGHT_OUTPUT_INVERT false
#define DISPLAY_INVERT_COLOR true
#define DISPLAY_RGB_ORDER LCD_RGB_ELEMENT_ORDER_RGB
#define DISPLAY_MIRROR_X false
#define DISPLAY_MIRROR_Y true
#define DISPLAY_SWAP_XY true
#define DISPLAY_OFFSET_X 0
#define DISPLAY_OFFSET_Y 0

//
// === 7. 预留：电源管理 & 其它扩展模块 ===
//
// 可以在此处添加电池检测 ADC 引脚、充电状态检测等宏定义

#endif // _BOARD_CONFIG_H_
