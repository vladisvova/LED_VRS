/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file           : main.c
 * @brief          : Main program body
 ******************************************************************************
 * @attention
 *
 * <h2><center>&copy; Copyright (c) 2019 STMicroelectronics.
 * All rights reserved.</center></h2>
 *
 * This software component is licensed by ST under BSD 3-Clause license,
 * the "License"; You may not use this file except in compliance with the
 * License. You may obtain a copy of the License at:
 *                        opensource.org/licenses/BSD-3-Clause
 *
 ******************************************************************************
 */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "dma.h"
#include "usart.h"
#include "gpio.h"
#include "stdbool.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

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

enum modes {MANUAL,AUTO};
enum states {RECEIVING,IGNORING};
enum count {UP,DOWN};
enum modes mode = AUTO;
bool change = false;

/* USER CODE BEGIN PV */
uint8_t tx_data[] = "Data to send over UART DMA!\n\r";
uint8_t rx_data[10];
uint16_t  PWM = 0;


/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */
void receive_dma_data(const uint8_t* data, uint16_t len);
void setDutyCycle(uint8_t D);
void fadeInOut();
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

	LL_APB2_GRP1_EnableClock(LL_APB2_GRP1_PERIPH_SYSCFG);
	LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_PWR);

	NVIC_SetPriorityGrouping(NVIC_PRIORITYGROUP_4);

	/* System interrupt init*/

	/* USER CODE BEGIN Init */

	/* USER CODE END Init */

	/* Configure the system clock */
	SystemClock_Config();

	/* USER CODE BEGIN SysInit */

	/* USER CODE END SysInit */

	/* Initialize all configured peripherals */
	MX_GPIO_Init();
	MX_DMA_Init();
	MX_USART2_UART_Init();
	MX_TIM2_Init();
	/* USER CODE BEGIN 2 */
	USART2_RegisterCallback(receive_dma_data);
	LL_TIM_CC_EnableChannel(TIM2, LL_TIM_CHANNEL_CH1);
	LL_TIM_EnableCounter(TIM2);

	/* USER CODE END 2 */

	/* Infinite loop */
	/* USER CODE BEGIN WHILE */
	while (1)
	{
		/* USER CODE END WHILE */

		USART2_CheckDmaReception();
		LL_mDelay(10);

		if(mode == MANUAL){

			if(change)
				LL_TIM_OC_SetCompareCH1(TIM2, 0);

			setPWM(PWM);
		}else
		{

			fadeInOut();

		}


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
	LL_FLASH_SetLatency(LL_FLASH_LATENCY_0);

	if(LL_FLASH_GetLatency() != LL_FLASH_LATENCY_0)
	{
		Error_Handler();
	}
	LL_RCC_HSI_Enable();

	/* Wait till HSI is ready */
	while(LL_RCC_HSI_IsReady() != 1)
	{

	}
	LL_RCC_HSI_SetCalibTrimming(16);
	LL_RCC_SetAHBPrescaler(LL_RCC_SYSCLK_DIV_1);
	LL_RCC_SetAPB1Prescaler(LL_RCC_APB1_DIV_1);
	LL_RCC_SetAPB2Prescaler(LL_RCC_APB1_DIV_1);
	LL_RCC_SetSysClkSource(LL_RCC_SYS_CLKSOURCE_HSI);

	/* Wait till System clock is ready */
	while(LL_RCC_GetSysClkSource() != LL_RCC_SYS_CLKSOURCE_STATUS_HSI)
	{

	}
	LL_Init1msTick(8000000);
	LL_SYSTICK_SetClkSource(LL_SYSTICK_CLKSOURCE_HCLK);
	LL_SetSystemCoreClock(8000000);
}

/* USER CODE BEGIN 4 */
void setDutyCycle(uint8_t D){

	uint8_t pulse_length = (LL_TIM_GetAutoReload(TIM2)* D) / 100;
	LL_TIM_OC_SetCompareCH1(TIM2, pulse_length);

}

void setPWM(uint8_t pwm){



	static uint8_t old_PWM = 0;

	if(change == true)
		old_PWM = 0;

	if(old_PWM<pwm){

		for(int i = 1; i<=pwm-old_PWM;i++){

			setDutyCycle(old_PWM+i);
			LL_mDelay(10);
		}

	}else if(old_PWM>pwm){

		for(int i = 1; i<=old_PWM-pwm;i++){

			setDutyCycle(old_PWM-i);
			LL_mDelay(10);
		}

	} else if(old_PWM==pwm){



	}


	change=false;
	old_PWM=pwm;
}

void fadeInOut(){

	static enum count counting = UP;
	static uint8_t old_PWM = 0;

	if(counting == UP && old_PWM == 100){
		counting = DOWN;
	} else 	if(counting == DOWN && old_PWM == 0){
		counting = UP;
	}

	if(counting == UP){
		old_PWM += 1;
		setDutyCycle(old_PWM);
	} else 	if(counting == DOWN){
		old_PWM -= 1;
		setDutyCycle(old_PWM);
	}



}



uint8_t process_pwm_data(uint8_t sign_1,uint8_t sign_2){


	char myarray[2] = {sign_1,sign_2};
	int pwm;
	sscanf(myarray, "%d", &pwm);

	return pwm;

}


void receive_dma_data(const uint8_t* data, uint16_t len)
{


	static enum states state = IGNORING;
	static uint16_t PWM_info[6];
	static uint16_t PWM_array_position = 0;


	for(uint8_t i = 0; i < len; i++)
	{

		if(mode == MANUAL){

			if(*(data+i) == '$' && state == IGNORING)
			{

				state=RECEIVING;
			} else if(state == RECEIVING && PWM_array_position<5){

				PWM_info[PWM_array_position++]=*(data+i);

			}

			else if(state == RECEIVING && PWM_array_position==5 && *(data+i) == '$' ){

				PWM_array_position = 0;
				state = IGNORING;
				PWM = process_pwm_data(PWM_info[3],PWM_info[4]);

			} else if(state == RECEIVING && PWM_array_position>4 ){

				PWM_array_position = 0;
				state = IGNORING;
			}

			if(state == RECEIVING && PWM_info[0] == 'a' && PWM_info[1] == 'u' && PWM_info[2] == 't' && PWM_info[3] == 'o' && PWM_info[4] == '$'){

				mode = AUTO;
				PWM_array_position = 0;
				state = IGNORING;

			}
		} else
		{


			if(*(data+i) == '$' && state == IGNORING)
			{
				state=RECEIVING;
			} else if(state == RECEIVING && PWM_array_position<6){

				PWM_info[PWM_array_position++]=*(data+i);


			}

			else if(state == RECEIVING && PWM_info[0] == 'm' && PWM_info[1] == 'a' && PWM_info[2] == 'n' && PWM_info[3] == 'u' && PWM_info[4] == 'a' && PWM_info[5] == 'l'&& *(data+i) == '$' ){

				PWM_array_position = 0;
				state = IGNORING;
				mode = MANUAL;;
				change = true; //zmena stavu z auto na manual, sluzi ako pomocna premena na prepis starej hodnoty PWM na 0

			} else if(state == RECEIVING && PWM_array_position>5 ){

				PWM_array_position = 0;
				state = IGNORING;
			}

		}
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
void assert_failed(char *file, uint32_t line)
{
	/* USER CODE BEGIN 6 */
	/* User can add his own implementation to report the file name and line number,
     tex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
	/* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
