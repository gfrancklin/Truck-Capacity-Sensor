/*!
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "stm32f3xx_hal.h"
#include "i2c.h"
#include "spi.h"
#include "gpio.h"
#include "max_i2cxl.h"
#ifdef USE_USB_DEBUG
#include "usb_device.h"
#include "usbd_cdc_if.h"
#endif

/* Function Prototypes --------------------------------------------------------*/
void SystemClock_Config(void);
void print_menu(void);

#ifdef USE_USB_DEBUG
uint8_t rx_data_buf[MAX_BUF_SIZE];
uint32_t data_len = 0;
#endif

/* GAB: This variable holds the current address of the one sensor currently supported.
 * In the next versions of the software, the typedef max_i2cxl will contain a last
 * measurement value, that will replace this variable. */
volatile int range_cm = 0;

/**
  * @brief  The application entry point.
  *
  * @retval None
  */
int main(void) {
	volatile uint32_t ret_val = 1;
	uint32_t last_trigger_time = HAL_GetTick();

#ifdef USE_USB_DEBUG
	char error_message[MAX_BUF_SIZE];
	memset(error_message, 0, MAX_BUF_SIZE);
#endif

	HAL_Init();
	SystemClock_Config();
	MX_GPIO_Init();
	MX_SPI3_Init();
	MX_I2C1_Init();

#ifdef USE_USB_DEBUG
	MX_USB_DEVICE_Init();

	print_stamp();
	print_debug_message("I: Initializing System...\r\n");
	print_debug_message("I: Initializing Sensor...\r\n");
#endif
	if((ret_val = i2cxl_maxsonar_init()) != 0) {
#ifdef USE_USB_DEBUG
		sprintf(error_message, "E: Error %d initializing sensor...\r\n", (int)ret_val);
		print_debug_message(error_message);
#endif
	}
#ifdef USE_USB_DEBUG
	print_debug_message("Initialization successful...\r\n");
	print_menu();
#endif

	/* Initialize FT800 Display */
	set_display_parameters();
	wake_up_display();
	initial_display_screen();

	while (1) {
#ifdef USE_USB_DEBUG
		if(data_len > 0) {
			CDC_Transmit_FS(rx_data_buf, data_len);
			switch (rx_data_buf[0]) {
		  		case '1' : {
		  			while(((ret_val != 0) || ((HAL_GetTick() - last_trigger_time) > TRIGGER_TIME_MS)) &&
								  rx_data_buf[data_len-1] != 'q') {
						 last_trigger_time = HAL_GetTick();
						 ret_val = i2cxl_maxsonar_start(112, &range_cm);
						 print_measurement(range_cm);
					  }
					  break;
				  }
				  case '2' : {
					  int old_addr = 112;
					  int new_addr = 10;
					  while(ret_val != 0) {
						  ret_val = i2cxl_maxsonar_change_addr(old_addr, &new_addr);
					  }
					  char buffer[255];
					  sprintf(buffer, "Address Changed to %d\r\n", new_addr);
					  print_debug_message(buffer);
					  break;
				  }
				  default : {
					  print_debug_message("\r\nNo Such Option\r\n");
					  break;
				  }
			  }
			  print_menu();
			  data_len = 0;
		}
#endif
#ifndef USE_USB_DEBUG
	while((ret_val != 0) || ((HAL_GetTick() - last_trigger_time) > TRIGGER_TIME_MS)) {
		last_trigger_time = HAL_GetTick();
		ret_val = i2cxl_maxsonar_start(112, &range_cm);
		draw_screen();
	}
#endif
	}
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void) {
  RCC_OscInitTypeDef RCC_OscInitStruct;
  RCC_ClkInitTypeDef RCC_ClkInitStruct;
  RCC_PeriphCLKInitTypeDef PeriphClkInit;

    /**Initializes the CPU, AHB and APB busses clocks 
    */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI|RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = 16;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL6;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    _Error_Handler(__FILE__, __LINE__);
  }

    /**Initializes the CPU, AHB and APB busses clocks 
    */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_1) != HAL_OK)
  {
    _Error_Handler(__FILE__, __LINE__);
  }

  PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_USB|RCC_PERIPHCLK_I2C1;
  PeriphClkInit.I2c1ClockSelection = RCC_I2C1CLKSOURCE_HSI;
  PeriphClkInit.USBClockSelection = RCC_USBCLKSOURCE_PLL;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
  {
    _Error_Handler(__FILE__, __LINE__);
  }

    /**Configure the Systick interrupt time 
    */
  HAL_SYSTICK_Config(HAL_RCC_GetHCLKFreq()/1000);

    /**Configure the Systick 
    */
  HAL_SYSTICK_CLKSourceConfig(SYSTICK_CLKSOURCE_HCLK);

  /* SysTick_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(SysTick_IRQn, 0, 0);
}

void print_menu(void) {
#ifdef USE_USB_DEBUG
	print_debug_message("\n\n\r--------- Main Menu -----------\r\n");
	print_debug_message("-------------------------------\r\n");
	print_debug_message("1. Measure distance\r\n");
	print_debug_message("2. Change Sensor Address\r\n");
#endif
}

/**
  * @brief  This function is executed in case of error occurrence.
  * @param  file: The file name as string.
  * @param  line: The line in file as a number.
  * @retval None
  */
void _Error_Handler(char *file, int line)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  while(1) 
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
void assert_failed(uint8_t* file, uint32_t line)
{ 
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
    ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */

/**
  * @}
  */

/**
  * @}
  */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
