/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2023 STMicroelectronics.
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
#include "dma.h"
#include "i2c.h"
#include "rtc.h"
#include "tim.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "stddef.h"
#include "string.h"
#include "ssd1306.h"
#include "test.h"
#include "bitmap.h"
#include "horse_anim.h"
#include "stdio.h"
#include "stdbool.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */


motor_opreration_callbk Tasks_states[4];
RTC_TimeTypeDef sTime;
RTC_DateTypeDef sDate;

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define MOTOR_OFF	0b00000000
#define FULL	0b00000100
#define HALF	0b00000110
#define EMPTY	0b00000111

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
uint8_t message[16] = "Period Elapsed\n\r";
FlagStatus periodElapsed = RESET;
uint32_t count_timer=0;
uint32_t Systick_counter=0;

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */
void User_TIMPeriodElapsedCallback(TIM_HandleTypeDef *htim);
void set_time();

void intToStringWithAlignment(char* str, int num, int width, int isRightAligned);
typedef void (*motor_opreration_callbk)(void);
void Tank_Empty_cllbk();
void Tank_Low_cllbk();
void Tank_Half_cllbk();
void Tank_full_cllbk();
void trigger_motor_oprations(uint8_t Water_Level);
void Register_Motor_oprtn_cllbk(uint8_t Task_function, motor_opreration_callbk Task_states);

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

//	char Oled_msg_str[6]="0";
//	uint8_t Chng_Lvl_state =0x00;
	uint8_t tribit_lvl_read=0x00;

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
  MX_DMA_Init();
  MX_I2C1_Init();
  MX_USART1_UART_Init();
  MX_RTC_Init();
  MX_TIM1_Init();
  /* USER CODE BEGIN 2 */
  HAL_TIM_RegisterCallback(&htim1, HAL_TIM_PERIOD_ELAPSED_CB_ID, User_TIMPeriodElapsedCallback);
  HAL_TIM_Base_Start_IT(&htim1);
  SSD1306_Init();
  HAL_RTC_Init(&hrtc);


  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */

  dip_startup();
  set_time();

  while (1)
  {
	  PutDT_Oled();
	  Register_Motor_oprtn_cllbk(MOTOR_OFF, Tank_full_cllbk);
	  Register_Motor_oprtn_cllbk(FULL, Tank_Half_cllbk);
	  Register_Motor_oprtn_cllbk(HALF, Tank_Low_cllbk);
	  Register_Motor_oprtn_cllbk(EMPTY, Tank_Empty_cllbk);
		tribit_lvl_read|=((HAL_GPIO_ReadPin(GPIOA,  Level_0_Pin)<<2)| (HAL_GPIO_ReadPin(GPIOA,  Level_1_Pin)<<1)| HAL_GPIO_ReadPin(GPIOA,  Level_2_Pin)<<0);
		trigger_motor_oprations(tribit_lvl_read);
		tribit_lvl_read=0x00;



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

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE2);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI|RCC_OSCILLATORTYPE_LSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.LSIState = RCC_LSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = 8;
  RCC_OscInitStruct.PLL.PLLN = 60;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 4;
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
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV4;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_1) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */
/*******************User---Functions************************************************************/

void User_TIMPeriodElapsedCallback(TIM_HandleTypeDef *htim){

	//HAL_UART_Transmit_DMA(&huart1,(uint8_t*) Serial_tx, strlen(Serial_tx));
	HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13);

}

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin){
	//HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_15);
	NVIC_SystemReset();
}

// 4 callbacks  so array to register callbacks



void Register_Motor_oprtn_cllbk(uint8_t Task_function, motor_opreration_callbk callback){

		Tasks_states[Task_function]=callback;

}

void trigger_motor_oprations(uint8_t Water_Level){
	if(Water_Level == MOTOR_OFF){
	Tasks_states[MOTOR_OFF]();
	}
	if(Water_Level == FULL){
	Tasks_states[FULL]();
	}
	if(Water_Level == HALF){
	Tasks_states[HALF]();
	}
	if(Water_Level == EMPTY){
	Tasks_states[EMPTY]();
	}

}

void Tank_full_cllbk(){
	//Motor OFF
	//show Full
	HAL_GPIO_WritePin(GPIOC, GPIO_PIN_15, false);
}
void Tank_Half_cllbk(){
	// Show Half
}

void Tank_Low_cllbk(){
	//Show Low
}
void Tank_Empty_cllbk(){
	//Motor ON
	//show Empty
	HAL_GPIO_WritePin(GPIOC, GPIO_PIN_15, true);
}

void set_time (){



  sTime.Hours = 0x7; // set hours
  sTime.Minutes = 0x00; // set minutes
  sTime.Seconds = 0x00; // set seconds
  sTime.DayLightSaving = RTC_DAYLIGHTSAVING_NONE;
  sTime.StoreOperation = RTC_STOREOPERATION_RESET;
  if (HAL_RTC_SetTime(&hrtc, &sTime, RTC_FORMAT_BCD) != HAL_OK)
  {
    Error_Handler();
  }
  sDate.WeekDay = RTC_MONTH_DECEMBER; //  day
  sDate.Month = 0x12; //   month
  sDate.Date = 0x25; // date
  sDate.Year = 0x23; // year
  if (HAL_RTC_SetDate(&hrtc, &sDate, RTC_FORMAT_BCD) != HAL_OK)
  {
    Error_Handler();
  }
  HAL_RTCEx_BKUPWrite(&hrtc, RTC_BKP_DR1, 0x32F2); // backup register
}



uint8_t binaryToBCD(uint8_t binary) {
    return ((binary / 10) << 4) | (binary % 10);
}
/********************-Extra Codes****************************
void auxilary_function(){
	//char Level_bit_str[3]="0";
	//int *Level_ptr=(int*)&Level_stat;
	//	int  Level_stat[3]={0,0,0};
	//		*Level_ptr=HAL_GPIO_ReadPin(GPIOA,  Level_0_Pin);
	//		*(Level_ptr+1)=HAL_GPIO_ReadPin(GPIOA,  Level_1_Pin);;
	//		*(Level_ptr+2)=HAL_GPIO_ReadPin(GPIOA,  Level_2_Pin);;
	//		intToStringWithAlignment( Level_bit_str,*Level_ptr, 1, 1);
	//		intToStringWithAlignment( &(*(Level_bit_str+1)), *(Level_ptr+1), 1, 1);
	//		intToStringWithAlignment( &(*(Level_bit_str+2)), *(Level_ptr+2), 1, 1);
	//		SSD1306_GotoXY (16,42);
	//		SSD1306_Puts(Level_bit_str, &Font_7x10, 1);
	//		SSD1306_UpdateScreen();
}

//		Chng_Lvl_state= MOTOR_OFF | tribit_lvl_read;
//		switch(Chng_Lvl_state){
//		case MOTOR_OFF:
//			//Turn On Higher critical task_1;
//			//Display Full-level
//			strcpy(Oled_msg_str, "Full ");
//
//			break;
//		case FULL:
//			//display MID-level
//			strcpy(Oled_msg_str, "Half");
//			break;
//		case HALF:
//			//display Low-level
//			strcpy(Oled_msg_str, "Low ");
//			break;
//		case EMPTY:
//			//Turn On Higher critical task_2;
//			strcpy(Oled_msg_str, "Empty");
//			break;
//		}
//
//		SSD1306_GotoXY (40,42);
//		SSD1306_Puts(Oled_msg_str, &Font_7x10, 1);
//		SSD1306_UpdateScreen();

**********************/

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
