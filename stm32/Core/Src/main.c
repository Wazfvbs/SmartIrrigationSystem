/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/


#include "main.h"
#include "adc.h"
#include "dma.h"
#include "i2c.h"
#include "tim.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "sensors.h"
#include "oled.h"
#include "dht11.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>       // CHANGED: include stdlib for atof()
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
#define DMA_BUF_SIZE  64
#define MAIN_BUF_SIZE 256

#define THIS_DEVICE_ID "MCU01"

/* GPIO for pump control */
#define PUMP_GPIO_PORT GPIOA
#define PUMP_GPIO_PIN  GPIO_PIN_8

/* USER CODE BEGIN PD */
/* CHANGED: 将 threshold 改为 float，以便使用 atof() */
static float g_wateringThreshold0 = 30.0f;   // CHANGED: float
static float g_wateringThreshold1 = 60.0f;   // CHANGED: float

static char g_deviceNickname[20] = "默认昵称";
static char g_plantSpecies[20]   = "未知";
static OledDisplayState g_oledState = OLED_STATE_NORMAL;
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */
uint8_t DHT11_Read_Data(uint8_t* humi,uint8_t* temp);
/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN PV */
static uint8_t DMA_Buffer[DMA_BUF_SIZE];  // 用于单次DMA接收
static uint8_t mainBuf[MAIN_BUF_SIZE];    // 主循环缓冲区，拼接命令
static uint16_t mainBufIndex = 0;         // 指向下一个待写入的位置
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);

/* USER CODE BEGIN PFP */
/* 函数声明 */
void parseCommand(const char* cmd);
void setPumpState(uint8_t on);
void updateOLEDState(OledDisplayState newState);
/* USER CODE END PFP */

/* USER CODE BEGIN 0 */

/*
 * CHANGED / ADDED:
 *  使用空闲中断回调方式，处理每次接收的数据块，并在其中查找换行符 '\n'
 *  如果找到，则分割出一条完整命令，调用 parseCommand() 解析
 */
void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t Size)
{

	if (huart == &huart1)
  {
    /* 1) 将 DMA_Buffer 中 [0..Size-1] 的字节 复制到 mainBuf */
    for(uint16_t i = 0; i < Size; i++)
    {
      // 如果满了，就清空或做其他处理，这里简单清空
      if(mainBufIndex >= MAIN_BUF_SIZE - 1)
      {
        mainBufIndex = 0;
        memset(mainBuf, 0, MAIN_BUF_SIZE);
      }
      mainBuf[mainBufIndex++] = DMA_Buffer[i];
    }
    /* 2) 在 mainBuf 中查找 '\n' 以判断是否有一条完整命令  */
    // CHANGED: 只要检测到第一条 '\n'，就解析并清空缓冲区，等待下条命令
    for(uint16_t i = 0; i < mainBufIndex; i++)
    {
      if(mainBuf[i] == '\n')
      {
        // 截取 [0 .. i-1] 作为一条命令   (startPos = 0)
        mainBuf[i] = '\0'; // 把换行符替换为字符串结束符

        // 提取该条命令
        char cmdBuf[100];
        uint16_t cmdLen = i; // 从0到i-1
        if(cmdLen >= sizeof(cmdBuf)) {
          cmdLen = sizeof(cmdBuf) - 1;
        }
        strncpy(cmdBuf, (char*)&mainBuf[0], cmdLen);
        cmdBuf[cmdLen] = '\0';
        // 调用 parseCommand() 来解析
        parseCommand(cmdBuf);

        // CHANGED / ADDED: 清空所有缓冲区并将 mainBufIndex 置0
        memset(mainBuf, 0, MAIN_BUF_SIZE);
        mainBufIndex = 0;

        // 因为你只想处理第一条指令就重置缓存，因此可以直接 break
        break;
      }
    }

    /* 3) 重新启动接收：非常重要，否则只能接收一次 */
    // CHANGED: 不再做剩余数据前移，因为你希望清空后只等下一条命令
    HAL_UARTEx_ReceiveToIdle_IT(&huart1, DMA_Buffer, DMA_BUF_SIZE);
  }
}

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  */
int main(void)
{
  /* USER CODE BEGIN 1 */
  uint8_t temperature = 1;
  uint8_t humidity    = 1;

  char* CntState = "No Connect!\r\n";
  uint8_t messages[250];
  Sensor_DataTypeDef sensorData;
  /* USER CODE END 1 */

  HAL_Init();
  SystemClock_Config();

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_TIM3_Init();
  MX_USART1_UART_Init();
  MX_I2C2_Init();
  MX_DMA_Init();
  MX_ADC1_Init();

  /* USER CODE BEGIN 2 */
  DHT11_Rst();

  while(DHT11_Check())
  {
    HAL_UART_Transmit(&huart1, (uint8_t*)CntState, strlen(CntState), 200);
    HAL_Delay(500);
  }

  CntState = "Success!\r\n";
  HAL_UART_Transmit(&huart1, (uint8_t*)CntState, strlen(CntState), 200);

  HAL_Delay(20);
  OLED_Init();
  SENSORS_Init();

  /* CHANGED / ADDED: 启动 USART1 的空闲中断接收 */

  HAL_UARTEx_ReceiveToIdle_IT(&huart1, DMA_Buffer, DMA_BUF_SIZE);

  /* USER CODE END 2 */

  while (1)
  {
    /* 读取传感器数据并打印示例 */
    SENSORS_ReadAll(&sensorData);

    //sprintf((char*)messages,"环境温度 %d ℃  环境湿度 %d %%  土壤湿度 %u %%  水位深度 %u cm  光照强度 %u\r\n",sensorData.temperature,sensorData.humidity,sensorData.soilADC, sensorData.waterADC,sensorData.lightADC);
    sprintf((char*)messages,"T=%d,H=%d,soil=%u,water=%u,light=%u\r\n",sensorData.temperature,sensorData.humidity,sensorData.soilADC, sensorData.waterADC,sensorData.lightADC);

    HAL_UART_Transmit(&huart1, messages, strlen((const char*)messages), 250);

    /* 可以再读 DHT11 ，如果需要 */
    DHT11_Read_Data(&humidity, &temperature);

    /* 刷新 OLED 状态 */
    OLED_UpdateDisplay(g_oledState, &sensorData, g_deviceNickname, g_plantSpecies);

    HAL_Delay(1000);
  }
}
/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
  RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }
  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
  PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_ADC;
  PeriphClkInit.AdcClockSelection = RCC_ADCPCLK2_DIV6;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */
// ------------------------------------------------
// 解析命令函数
// 格式: <DEVICE_ID>:<COMMAND_TYPE>:<DATA>
// �??: "MCU01:p1:" or "MCU01:oled:listen" or "MCU01:nickname:小花"
// ------------------------------------------------
void parseCommand(const char* cmd)
{
  // 1. 拆分字符�??
  //    假设�??单处理，使用 strtok 来分�?? ":"
  char buffer[100];
  strncpy(buffer, cmd, sizeof(buffer)-1);
  buffer[sizeof(buffer)-1] = '\0';
  // 分割得到 3 段： DEVICE_ID, COMMAND_TYPE, DATA
  char* pDevice  = strtok(buffer, ":");
  char* pCommand = strtok(NULL, ":");
  char* pData    = strtok(NULL, ":");

  if(pDevice == NULL || pCommand == NULL) {
    return; // 格式不正确，直接返回
  }

  // 2. 判断是否为本设备ID
  if(strcmp(pDevice, THIS_DEVICE_ID) != 0) {
    // 不是发给我们的，忽略
    return;
  }

  // 3. 根据 COMMAND_TYPE 判断执行动作
  if(strcmp(pCommand, "p1") == 0) {
    // 将PA8置高
    setPumpState(1);
    updateOLEDState(OLED_STATE_WATERING);
  }
  else if(strcmp(pCommand, "p0") == 0) {
    // 将PA15置低
    setPumpState(0);
    updateOLEDState(OLED_STATE_NORMAL);
  }
  else if(strcmp(pCommand, "oled") == 0) {
    // 切换OLED状�??
    // pData 可能�?? "normal", "listen", "talk", "water"
    if(pData) {
      if(strcmp(pData, "normal") == 0) {
        updateOLEDState(OLED_STATE_NORMAL);
      } else if(strcmp(pData, "listen") == 0) {
        updateOLEDState(OLED_STATE_LISTENING);
      } else if(strcmp(pData, "talk") == 0) {
        updateOLEDState(OLED_STATE_TALKING);
      } else if(strcmp(pData, "water") == 0) {
        updateOLEDState(OLED_STATE_WATERING);
      }
    }
  }
  else if(strcmp(pCommand, "threshold0") == 0) {
    // 设置浇水阈�??
    if(pData) {
      float newVal = atof(pData);
      g_wateringThreshold0 = newVal;
      char ack[50];
      sprintf(ack, "设置浇水下限设置为%u\r\n", g_wateringThreshold0);

      HAL_UART_Transmit(&huart1, (uint8_t*)ack, strlen(ack), HAL_MAX_DELAY);
    }
  }
  else if(strcmp(pCommand, "threshold1") == 0) {
      // 设置浇水阈�??
      if(pData) {
        float newVal = atof(pData);
        g_wateringThreshold1 = newVal;
        char ack[50];
        sprintf(ack, "设置浇水上限设置为%u\r\n", g_wateringThreshold1);

        HAL_UART_Transmit(&huart1, (uint8_t*)ack, strlen(ack), HAL_MAX_DELAY);
      }
    }
  else if(strcmp(pCommand, "nickname") == 0) {
    // 设置设备昵称
    if(pData) {
      strncpy(g_deviceNickname, pData, sizeof(g_deviceNickname)-1);
      g_deviceNickname[sizeof(g_deviceNickname)-1] = '\0';
      char ack[50];
      sprintf(ack, "昵称设置为%s\r\n", g_deviceNickname);
      HAL_UART_Transmit(&huart1, (uint8_t*)ack, strlen(ack), HAL_MAX_DELAY);
    }
  }
  else if(strcmp(pCommand, "plant") == 0) {
    // 设置植物类型
    if(pData) {
      strncpy(g_plantSpecies, pData, sizeof(g_plantSpecies)-1);
      g_plantSpecies[sizeof(g_plantSpecies)-1] = '\0';
      char ack[50];
      sprintf(ack, "植物设置为%s\r\n", g_plantSpecies);
      HAL_UART_Transmit(&huart1, (uint8_t*)ack, strlen(ack), HAL_MAX_DELAY);
    }
  }
  else {
    // 未知命令
    const char* unknown = "Unknown Command.\r\n";
    HAL_UART_Transmit(&huart1, (uint8_t*)unknown, strlen(unknown), HAL_MAX_DELAY);
  }
}

// --------------------------------------------
// 控制水泵引脚
// p1 -> high, p0 -> low
// --------------------------------------------
void setPumpState(uint8_t on)
{
  if(on) {
    HAL_GPIO_WritePin(PUMP_GPIO_PORT, PUMP_GPIO_PIN, GPIO_PIN_SET);
  } else {
    HAL_GPIO_WritePin(PUMP_GPIO_PORT, PUMP_GPIO_PIN, GPIO_PIN_RESET);
  }
}

// --------------------------------------------
// 更新全局OLED状�??
// --------------------------------------------
void updateOLEDState(OledDisplayState newState)
{
  g_oledState = newState;
}

// End of main.c
/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */

