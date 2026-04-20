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

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "DHT11.h"
#include "utils.h"
#include "HCSR04.h"
#include "ring_buffer.h"
#include "ir_remote.h"
#include "lcd.h"
#include <stdio.h>
#include <string.h>
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
I2C_HandleTypeDef hi2c1;

TIM_HandleTypeDef htim14;
TIM_HandleTypeDef htim16;
TIM_HandleTypeDef htim17;

UART_HandleTypeDef huart2;

/* USER CODE BEGIN PV */
static SystemMode_t current_mode = MODE_LIVE;
static uint8_t playback_index = 0;
static RingBuffer_t rb;
static uint16_t distance_threshold = 20;
static uint16_t temp_distance_threshold = 20;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_I2C1_Init(void);
static void MX_TIM14_Init(void);
static void MX_TIM16_Init(void);
static void MX_TIM17_Init(void);
static void MX_USART2_UART_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

void UART_Print(char *msg){
	HAL_UART_Transmit(&huart2, (uint8_t *)msg, strlen(msg), HAL_MAX_DELAY);
}

void Handle_IR_Command(IR_Data_t *ir_data){
	DataPoint_t display_data;
	char display_buf[64];
	uint16_t min_distance_threshold = 2;
	uint16_t max_distance_threshold = 400;
	uint8_t data_retrieved;

	switch(ir_data->command){
		case 0x0C:	//1 Live Mode
			playback_index = 0;
			current_mode = MODE_LIVE;
			UART_Print("Mode: LIVE\r\n");
			break;

		case 0x18:	//2 Playback Mode
			playback_index = 0;
			current_mode = MODE_PLAYBACK;
			UART_Print("Mode: PLAYBACK\r\n");
			LCD_Clear();
			LCD_Print("Mode: PLAYBACK");

			HAL_Delay(750);
			data_retrieved = RingBuffer_Read(&rb, playback_index, &display_data);
			if(data_retrieved){
				LCD_Clear();
				snprintf(display_buf, sizeof(display_buf), "[%d], T:%dC, H:%d%%",
						playback_index, display_data.temperature, display_data.humidity);
				LCD_Print(display_buf);

				LCD_SetCursor(1, 0);

				snprintf(display_buf, sizeof(display_buf), "D:%dcm, @%lus",
						display_data.distance, display_data.timestamp/1000);
				LCD_Print(display_buf);
			}

			break;

		case 0x5E:	//3 Alert Config Mode
			current_mode = MODE_ALERT_CONFIG;
			temp_distance_threshold = distance_threshold;
			UART_Print("Mode: ALERT CONFIG\r\n");

			LCD_Clear();
			LCD_Print("Mode:");
			LCD_SetCursor(1, 0);
			LCD_Print("ALERT_CONFIG");

			HAL_Delay(750);

			LCD_Clear();
			LCD_Print("Dist Threshold");
			LCD_SetCursor(1, 0);
			snprintf(display_buf, sizeof(display_buf), "%d", distance_threshold);
			LCD_Print(display_buf);
			break;

		case 0x16:	//0 Home
			playback_index = 0;
			current_mode = MODE_LIVE;
			UART_Print("Mode: LIVE\r\n");
			break;

		case 0x47:	//CH+ Next Entry In Playback
			if(current_mode == MODE_PLAYBACK){

				if(playback_index >= RingBuffer_GetCount(&rb) - 1){
					playback_index = 0;
				}else{
					playback_index++;
				}

				data_retrieved = RingBuffer_Read(&rb, playback_index, &display_data);
				if(data_retrieved){
					LCD_Clear();
					snprintf(display_buf, sizeof(display_buf), "[%d], T:%dC, H:%d%%",
							playback_index, display_data.temperature, display_data.humidity);
					LCD_Print(display_buf);

					LCD_SetCursor(1, 0);

					snprintf(display_buf, sizeof(display_buf), "D:%dcm, @%lus",
							display_data.distance, display_data.timestamp/1000);
					LCD_Print(display_buf);
					}
			}
			break;

		case 0x45:	//CH- Previous Entry In Playback
			if(current_mode == MODE_PLAYBACK){

				if(playback_index  == 0){
					playback_index = RingBuffer_GetCount(&rb) - 1;
				}else{
				playback_index--;
				}

				data_retrieved = RingBuffer_Read(&rb, playback_index, &display_data);
				if(data_retrieved){
					LCD_Clear();
					snprintf(display_buf, sizeof(display_buf), "[%d], T:%dC, H:%d%%",
							playback_index, display_data.temperature, display_data.humidity);
					LCD_Print(display_buf);

					LCD_SetCursor(1, 0);

					snprintf(display_buf, sizeof(display_buf), "D:%dcm, @%lus",
							display_data.distance, display_data.timestamp/1000);
					LCD_Print(display_buf);
					}
			}
			break;

		case 0x15:	//Vol+ Increase Threshold
			if(current_mode == MODE_ALERT_CONFIG){
				if(temp_distance_threshold < max_distance_threshold){
					temp_distance_threshold++;
					LCD_SetCursor(1, 0);
					snprintf(display_buf, sizeof(display_buf), "%-4d", temp_distance_threshold);
					LCD_Print(display_buf);
				}
			}
			break;

		case 0x07:	//Vol- Decrease Threshold
			if(current_mode == MODE_ALERT_CONFIG){
				if(temp_distance_threshold > min_distance_threshold){
					temp_distance_threshold--;
					LCD_SetCursor(1, 0);
					snprintf(display_buf, sizeof(display_buf), "%-4d", temp_distance_threshold);
					LCD_Print(display_buf);
				}
			}
			break;

		case 0x43:	//Play/Pause - Confirm
			if(current_mode == MODE_ALERT_CONFIG){
				distance_threshold = temp_distance_threshold;
				current_mode = MODE_LIVE;
			}
			break;

		default:
			break;
	}
}

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
  MX_I2C1_Init();
  MX_TIM14_Init();
  MX_TIM16_Init();
  MX_TIM17_Init();
  MX_USART2_UART_Init();
  /* USER CODE BEGIN 2 */
  utils_init(&htim14);
  DHT11_Init();
  HCSR04_Init(&htim16);
  IR_Init(&htim17);

  DHT11_Data_t dht_data;
  DHT11_Status_t dht_status;

  HCSR04_Data_t hcsr04_data;
  HCSR04_Status_t hcsr04_status;

  IR_Data_t ir_data;

  uint32_t last_hcsr04_tick = 0;
  uint32_t last_dht11_tick = 0;
  uint16_t last_distance = 0;
  RingBuffer_Init(&rb);

  char uart_buf[64];
  char lcd_buf[64];
  __HAL_TIM_SET_COUNTER(&htim14, 0);
  delay_us(100);

  HAL_Delay(500);
  LCD_Init(&hi2c1);
  LCD_Print("Data Logger Ready");
  LCD_SetCursor(1, 0);
  LCD_Print("Waiting...");

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */

  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */


	  if(IR_Get_Command(&ir_data)){
		  Handle_IR_Command(&ir_data);
	  }

	  uint32_t now = HAL_GetTick();

	  if(current_mode == MODE_LIVE){

	  //HCSR04 Data - Read Every 500ms
	  if(now - last_hcsr04_tick >= 500){

		  last_hcsr04_tick = now;

		  hcsr04_status = HCSR04_Read(&hcsr04_data);

		  if(hcsr04_status == HCSR04_OK){
			  last_distance = hcsr04_data.distance_cm;
		  }else{
			  snprintf(uart_buf, sizeof(uart_buf), "HCSR04 Err: %d\r\n", hcsr04_status);
			  UART_Print(uart_buf);
		  }
	  }

	  if(now - last_dht11_tick >= 2000){

		  last_dht11_tick = now;

		  dht_status = DHT11_ReadData(&dht_data);

		  if(dht_status == DHT11_OK){

			  DataPoint_t dp = {
					  .distance = last_distance,
					  .temperature = dht_data.temperature,
					  .humidity = dht_data.humidity,
					  .timestamp = now
			  };

			  if(last_distance < distance_threshold){
				  HAL_GPIO_WritePin(BUZZER_GPIO_Port, BUZZER_Pin, GPIO_PIN_SET);
			  }else{
				  HAL_GPIO_WritePin(BUZZER_GPIO_Port, BUZZER_Pin, GPIO_PIN_RESET);
			  }

			  RingBuffer_Write(&rb, &dp);

			  snprintf(uart_buf, sizeof(uart_buf), "T: %dC, H: %d %%, D: %dcm, @ %lu ms \r\n",
					  dp.temperature, dp.humidity, dp.distance, dp.timestamp);
			  UART_Print(uart_buf);

				  LCD_Clear();
				  snprintf(lcd_buf, sizeof(lcd_buf), "T:%dC H:%d%%",dp.temperature, dp.humidity);
				  LCD_Print(lcd_buf);
				  LCD_SetCursor(1, 0);
				  snprintf(lcd_buf, sizeof(lcd_buf), "D:%dcm @%lus",dp.distance, dp.timestamp/1000);
				  LCD_Print(lcd_buf);

		  }else{
			  snprintf(uart_buf, sizeof(uart_buf), "DHT11 Err: %d\r\n", dht_status);
			  UART_Print(uart_buf);
		  }


	  }
	  }
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

  /** Configure the main internal regulator output voltage
  */
  HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSIDiv = RCC_HSI_DIV1;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = RCC_PLLM_DIV1;
  RCC_OscInitStruct.PLL.PLLN = 8;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLR = RCC_PLLR_DIV2;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief I2C1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_I2C1_Init(void)
{

  /* USER CODE BEGIN I2C1_Init 0 */

  /* USER CODE END I2C1_Init 0 */

  /* USER CODE BEGIN I2C1_Init 1 */

  /* USER CODE END I2C1_Init 1 */
  hi2c1.Instance = I2C1;
  hi2c1.Init.Timing = 0x10B17DB5;
  hi2c1.Init.OwnAddress1 = 0;
  hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c1.Init.OwnAddress2 = 0;
  hi2c1.Init.OwnAddress2Masks = I2C_OA2_NOMASK;
  hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c1) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Analogue filter
  */
  if (HAL_I2CEx_ConfigAnalogFilter(&hi2c1, I2C_ANALOGFILTER_ENABLE) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Digital filter
  */
  if (HAL_I2CEx_ConfigDigitalFilter(&hi2c1, 0) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN I2C1_Init 2 */

  /* USER CODE END I2C1_Init 2 */

}

/**
  * @brief TIM14 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM14_Init(void)
{

  /* USER CODE BEGIN TIM14_Init 0 */

  /* USER CODE END TIM14_Init 0 */

  /* USER CODE BEGIN TIM14_Init 1 */

  /* USER CODE END TIM14_Init 1 */
  htim14.Instance = TIM14;
  htim14.Init.Prescaler = 63;
  htim14.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim14.Init.Period = 65535;
  htim14.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim14.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim14) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM14_Init 2 */

  /* USER CODE END TIM14_Init 2 */

}

/**
  * @brief TIM16 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM16_Init(void)
{

  /* USER CODE BEGIN TIM16_Init 0 */

  /* USER CODE END TIM16_Init 0 */

  /* USER CODE BEGIN TIM16_Init 1 */

  /* USER CODE END TIM16_Init 1 */
  htim16.Instance = TIM16;
  htim16.Init.Prescaler = 63;
  htim16.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim16.Init.Period = 65535;
  htim16.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim16.Init.RepetitionCounter = 0;
  htim16.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim16) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM16_Init 2 */

  /* USER CODE END TIM16_Init 2 */

}

/**
  * @brief TIM17 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM17_Init(void)
{

  /* USER CODE BEGIN TIM17_Init 0 */

  /* USER CODE END TIM17_Init 0 */

  /* USER CODE BEGIN TIM17_Init 1 */

  /* USER CODE END TIM17_Init 1 */
  htim17.Instance = TIM17;
  htim17.Init.Prescaler = 63;
  htim17.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim17.Init.Period = 65535;
  htim17.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim17.Init.RepetitionCounter = 0;
  htim17.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim17) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM17_Init 2 */

  /* USER CODE END TIM17_Init 2 */

}

/**
  * @brief USART2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART2_UART_Init(void)
{

  /* USER CODE BEGIN USART2_Init 0 */

  /* USER CODE END USART2_Init 0 */

  /* USER CODE BEGIN USART2_Init 1 */

  /* USER CODE END USART2_Init 1 */
  huart2.Instance = USART2;
  huart2.Init.BaudRate = 115200;
  huart2.Init.WordLength = UART_WORDLENGTH_8B;
  huart2.Init.StopBits = UART_STOPBITS_1;
  huart2.Init.Parity = UART_PARITY_NONE;
  huart2.Init.Mode = UART_MODE_TX_RX;
  huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart2.Init.OverSampling = UART_OVERSAMPLING_16;
  huart2.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  huart2.Init.ClockPrescaler = UART_PRESCALER_DIV1;
  huart2.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
  if (HAL_UART_Init(&huart2) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UARTEx_SetTxFifoThreshold(&huart2, UART_TXFIFO_THRESHOLD_1_8) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UARTEx_SetRxFifoThreshold(&huart2, UART_RXFIFO_THRESHOLD_1_8) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UARTEx_DisableFifoMode(&huart2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART2_Init 2 */

  /* USER CODE END USART2_Init 2 */

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  /* USER CODE BEGIN MX_GPIO_Init_1 */

  /* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOA, DHT11_DATA_Pin|HCSR04_TRIG_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(BUZZER_GPIO_Port, BUZZER_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin : DHT11_DATA_Pin */
  GPIO_InitStruct.Pin = DHT11_DATA_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(DHT11_DATA_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : HCSR04_TRIG_Pin */
  GPIO_InitStruct.Pin = HCSR04_TRIG_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
  HAL_GPIO_Init(HCSR04_TRIG_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : HCSR04_ECHO_Pin */
  GPIO_InitStruct.Pin = HCSR04_ECHO_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(HCSR04_ECHO_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : BUZZER_Pin */
  GPIO_InitStruct.Pin = BUZZER_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(BUZZER_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : IR_INPUT_Pin */
  GPIO_InitStruct.Pin = IR_INPUT_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(IR_INPUT_GPIO_Port, &GPIO_InitStruct);

  /* EXTI interrupt init*/
  HAL_NVIC_SetPriority(EXTI4_15_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI4_15_IRQn);

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */

void HAL_GPIO_EXTI_Falling_Callback(uint16_t GPIO_Pin){
	  if(GPIO_Pin == IR_INPUT_Pin){
		  IR_EXTI_Callback();
	  }
}

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
