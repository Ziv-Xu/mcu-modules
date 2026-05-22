/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
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
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "oled.h"
#include "pca9685.h"
#include "soft_i2c_simple.h"
#include "stm32f1xx_hal.h"

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  /* USER CODE BEGIN 2 */
SoftI2C_Init(&i2c_oled);
SoftI2C_Init(&i2c_pca9685);
OLED_Init(); // OLED初始化

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */

// 初始化 PCA9685（自动设置50Hz，复位，清空所有通道）
pca9685_Init();

// 在 pca9685_Init() 之后添加
uint8_t mode1 = 0;
SoftI2C_Start(&i2c_pca9685);
SoftI2C_SendByte(&i2c_pca9685, PCA9685_ADDR);   // 注意：现在 PCA9685_ADDR=0x70
SoftI2C_WaitAck(&i2c_pca9685);
SoftI2C_SendByte(&i2c_pca9685, 0x00);           // MODE1 寄存器地址
SoftI2C_WaitAck(&i2c_pca9685);
SoftI2C_Start(&i2c_pca9685);
SoftI2C_SendByte(&i2c_pca9685, PCA9685_ADDR | 0x01); // 读地址
SoftI2C_WaitAck(&i2c_pca9685);
mode1 = SoftI2C_ReadByte(&i2c_pca9685, 0);      // 读一个字节，NACK
SoftI2C_Stop(&i2c_pca9685);
OLED_ShowHexNum(2, 1, mode1, 2);  // 显示在第2行
HAL_Delay(1000);

uint8_t mode2 = pca_read_reg(PCA9685_MODE2);
OLED_ShowHexNum(4, 1, mode2, 2);

I2C_ScanBus(&i2c_pca9685);
HAL_Delay(1000);
OLED_Clear();

uint8_t off_l2 = pca_read_reg(0x08);
uint8_t off_h2 = pca_read_reg(0x09);
OLED_ShowHexNum(4, 1, off_l2, 2);
OLED_ShowHexNum(4, 8, off_h2, 2);

OLED_ShowString(1, 1, "PCA9685 OK");
HAL_Delay(1000);
OLED_Clear();
// 设置通道0：ON=0, OFF=2048 (50% 占空比)
pca9685_SetPWM(0, 2048);
pca9685_SetPWM(1, 2048);
pca9685_SetPWM(2, 2048);
pca9685_SetPWM(3, 2048); 
pca9685_SetPWM(4, 2048);
pca9685_SetPWM(5, 2048);
pca9685_SetPWM(6, 2048);
pca9685_SetPWM(7, 2048);
pca9685_SetPWM(8, 2048);
pca9685_SetPWM(9, 2048);
pca9685_SetPWM(10, 2048);
pca9685_SetPWM(11, 2048);
pca9685_SetPWM(12, 2048);
pca9685_SetPWM(13, 2048);
pca9685_SetPWM(14, 2048);
pca9685_SetPWM(15, 2048);
uint8_t off_l = pca_read_reg(0x08);
uint8_t off_h = pca_read_reg(0x09);
OLED_ShowHexNum(4, 1, off_l, 2);
OLED_ShowHexNum(4, 8, off_h, 2);
HAL_Delay(1000);
OLED_Clear();
/* USER CODE END 2 */

/* 无限循环 */
while (1)
{
    // 通道 0 从 0° 转到 180°
    for (uint8_t angle = 0; angle <= 180; angle += 5)   // 步进5度，减少抖动
    {
        pca9685_SetServoAngle(0, angle);
				pca9685_SetServoAngle(1, angle);
				pca9685_SetServoAngle(2, angle);
				pca9685_SetServoAngle(3, angle);
				pca9685_SetServoAngle(4, angle);
				pca9685_SetServoAngle(5, angle);
				pca9685_SetServoAngle(6, angle);
				pca9685_SetServoAngle(7, angle);
				pca9685_SetServoAngle(8, angle);
				pca9685_SetServoAngle(9, angle);
				pca9685_SetServoAngle(10, angle);
				pca9685_SetServoAngle(11, angle);
				pca9685_SetServoAngle(12, angle);
				pca9685_SetServoAngle(13, angle);
				pca9685_SetServoAngle(14, angle);
				pca9685_SetServoAngle(15, angle);
        OLED_ShowString(1, 1, "CH0:");
        OLED_ShowNum(1, 5, 0, 1);
        OLED_ShowString(1, 7, "Angle:");
        OLED_ShowNum(1, 13, angle, 3);
        HAL_Delay(1);
    }
    HAL_Delay(500);

    // 通道 0 从 180° 转回 0°
    for (uint8_t angle = 180; angle > 0; angle -= 5)
    {
        pca9685_SetServoAngle(0, angle);
				pca9685_SetServoAngle(1, angle);
				pca9685_SetServoAngle(2, angle);
				pca9685_SetServoAngle(3, angle);
				pca9685_SetServoAngle(4, angle);
				pca9685_SetServoAngle(5, angle);
				pca9685_SetServoAngle(6, angle);
				pca9685_SetServoAngle(7, angle);
				pca9685_SetServoAngle(8, angle);
				pca9685_SetServoAngle(9, angle);
				pca9685_SetServoAngle(10, angle);
				pca9685_SetServoAngle(11, angle);
				pca9685_SetServoAngle(12, angle);
				pca9685_SetServoAngle(13, angle);
				pca9685_SetServoAngle(14, angle);
				pca9685_SetServoAngle(15, angle);
        OLED_ShowString(1, 1, "CH0:");
        OLED_ShowNum(1, 5, 0, 1);
        OLED_ShowString(1, 7, "Angle:");
        OLED_ShowNum(1, 13, angle, 3);
        HAL_Delay(1);
    }
    HAL_Delay(500);

    // 如果你想测试其它通道（比如通道1），取消下面注释
    HAL_Delay(500);
    

    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

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
}

/* USER CODE BEGIN 4 */

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
#ifdef USE_FULL_ASSERT
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
