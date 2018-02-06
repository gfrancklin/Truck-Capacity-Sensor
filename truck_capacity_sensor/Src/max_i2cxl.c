/*!
 *****************************************************************
 * \author: Gabriel Yano
 * \date: Feb 2, 2018
 * \file: max_i2cxl.c
 * 		This file contains a library for the STM32F3 to handle
 * 		the MAXSONAR I2C MB12XX Series
 *****************************************************************
 */

/* Includes ******************************************************/
#include "stdint.h"
#include "stdio.h"
#include "stm32f3xx_hal.h"

/* Defines *******************************************************/
#define	CLEAR_BIT_0			0xFE
#define	SET_BIT_0			0x01
#define RX_BUFFER_SIZE		2
#define	MAX_ATTEMPTS		3
#define READ_RANGE_CMD		0x51
#define ADDR_UNLOCK_1_CMD	0xAA
#define ADDR_UNLOCK_2_CMD	0xA5
#define TX_BUFFER_SIZE		1
#define TIMEOUT_MS			1000
#define MAX_ADDR_VALUE		0xFF

/* Global Variables *********************************************/
I2C_HandleTypeDef hi2c1;

/* Function Prototypes ******************************************/
int i2cxl_maxsonar_init(void);
static int32_t i2cxl_maxsonar_measure(int);
static int32_t i2cxl_maxsonar_read(int, int *);
int32_t i2cxl_maxsonar_change_addr(int, int *);

/*!
 * \author Gabriel Yano
 * \name i2cxl_maxsonar_init
 * \brief Initializes the I2C network and join as a master device
 * \param none
 * \retval none
 * \note This function must be called prior to the use of the sensor
 */
int i2cxl_maxsonar_init(void) {
	  hi2c1.Instance = I2C1;
	  hi2c1.Init.Timing = 0x2000090E;
	  hi2c1.Init.OwnAddress1 = 0;
	  hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
	  hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
	  hi2c1.Init.OwnAddress2 = 0;
	  hi2c1.Init.OwnAddress2Masks = I2C_OA2_NOMASK;
	  hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
	  hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
	  if (HAL_I2C_Init(&hi2c1) != 0) {
	    return -1;
	  }

	    /**Configure Analogue filter
	    */
	  if (HAL_I2CEx_ConfigAnalogFilter(&hi2c1, I2C_ANALOGFILTER_ENABLE) != 0) {
	    return -2;
	  }

	    /**Configure Digital filter
	    */
	  if (HAL_I2CEx_ConfigDigitalFilter(&hi2c1, 0) != 0) {
		return -3;
	  }
	  return 0;
}

/*!
 * \author Gabriel Yano
 * \name i2cxl_maxsonar_start
 * \brief Triggers a measurement and reads back the range
 * \param addr - Address of the sensor in the I2C network
 * \param range_cm - pointer to where to store the value read
 * \retval 0 for success
 * \retval error code if error
 * \note The error codes follow the functions measure and read return values.
 * 		 In addition, it sets the MSB if error during the measurement or
 * 		 clear it if error during the read.
 */
int32_t i2cxl_maxsonar_start(int addr, int * range_cm) {
	int32_t ret_val = 0;

	/* If some error occur it will set or clear the MSB to flag weather error
	 * in measurement or reading */
	if((ret_val = i2cxl_maxsonar_measure(addr)) != 0) {
		ret_val |= 0x80;
	} else if((ret_val = i2cxl_maxsonar_read(addr, range_cm)) != 0) {
		ret_val &= 0x7F;
	}
	return ret_val;
}

/*!
 * \author Gabriel Yano
 * \name i2cxl_maxsonar_measure
 * \brief Triggers a measurement
 * \param addr - Address of the sensor in the I2C network
 * \retval 0 for success
 * \retval -1 if address bigger than a 8 bits address
 * \retval -2 if couldn't transmit to the device
 * \note
 */
static int32_t i2cxl_maxsonar_measure(int addr) {
	int32_t ret_val = 0;
	uint8_t tx_cmd = READ_RANGE_CMD;
	uint8_t retry_times = 0;

	if(addr > MAX_ADDR_VALUE) {	//the address can't be higher than 255
		ret_val = -1;
	} else {
		addr = addr & CLEAR_BIT_0;
		while((ret_val != 0) | (retry_times < MAX_ATTEMPTS)) {
			retry_times++;
			ret_val = HAL_I2C_Master_Transmit(&hi2c1, (uint16_t)addr, &tx_cmd, sizeof(tx_cmd), TIMEOUT_MS);
		}
		if(retry_times == MAX_ATTEMPTS) {
			ret_val = -2;
		}
	}
	return ret_val;
}

/*!
 * \author Gabriel Yano
 * \name i2cxl_maxsonar_read
 * \brief Reads the last measures distanced
 * \param addr - Address of the sensor in the I2C network
 * \param range_cm - pointer to where to store the value read
 * \retval 0 for success
 * \retval -1 if address bigger than a 8 bits address
 * \retval -2 if couldn't transmit to the device
 * \note
 */
static int32_t i2cxl_maxsonar_read(int addr, int * range_cm) {
	int32_t ret_val = 0;
	uint8_t rx_buffer[RX_BUFFER_SIZE] = {0};
	uint8_t retry_times = 0;

	if(addr > MAX_ADDR_VALUE) {	//the address can't be higher than 255
		ret_val = -1;
	} else {
		addr = addr | SET_BIT_0;	//set bit 0 to start reading

		/* Try to communicate for three times */
		while((ret_val != 0) | (retry_times < MAX_ATTEMPTS)) {
			retry_times++;
			ret_val = HAL_I2C_Master_Receive(&hi2c1, (uint16_t)addr, &rx_buffer[0], RX_BUFFER_SIZE, TIMEOUT_MS);
		}
		if(retry_times == MAX_ATTEMPTS) {
			ret_val = -2;
		} else {
			*range_cm = (rx_buffer[0] << 8) | rx_buffer[1];
		}
	}
	return ret_val;
}

/*!
 * \author Gabriel Yano
 * \name i2cxl_maxsonar_change_addr
 * \brief Changes the address of the device in the I2C Network
 * \param old_addr - Current address of the device
 * \param new_addr - pointer storing the new address to set
 * \retval 0 for success
 * \retval -1 if address bigger than a 8 bits address
 * \retval -2 if one of the invalid addresses
 * \retval -3 if couldn't transmit to the device
 * \note
 */
int32_t i2cxl_maxsonar_change_addr(int old_addr, int * new_addr) {
	int32_t ret_val = 0;
	uint32_t retry_times = 0;
	uint8_t tx_cmd = ADDR_UNLOCK_1_CMD;

	if((old_addr > MAX_ADDR_VALUE) | (*new_addr > MAX_ADDR_VALUE)) {	//the address can't be higher than 255
		ret_val = -1;
	} else if((*new_addr == 0x00) | (*new_addr == 80) | (*new_addr == 164) | (*new_addr == 170)) {
		ret_val = -2;
	} else {
		if((*new_addr % 2) == 1) {
			*new_addr = *new_addr - 1;
		}
		old_addr = old_addr & CLEAR_BIT_0;

		while((ret_val != 0) | (retry_times < MAX_ATTEMPTS)) {
			retry_times++;
			ret_val = HAL_I2C_Master_Transmit(&hi2c1, (uint16_t)old_addr, &tx_cmd, sizeof(tx_cmd), TIMEOUT_MS);
			if(ret_val == 0) {
				tx_cmd = ADDR_UNLOCK_2_CMD;
				ret_val = HAL_I2C_Master_Transmit(&hi2c1, (uint16_t)old_addr, &tx_cmd, sizeof(tx_cmd), TIMEOUT_MS);
				if(ret_val == 0) {
					ret_val = HAL_I2C_Master_Transmit(&hi2c1, (uint16_t)old_addr, (uint8_t *)new_addr, sizeof(new_addr), TIMEOUT_MS);
				}
			}
		}
		if(retry_times == MAX_ATTEMPTS) {
			ret_val = -3;
		} else {
			ret_val = HAL_I2C_IsDeviceReady(&hi2c1, *(uint16_t *)new_addr, MAX_ATTEMPTS, TIMEOUT_MS);
		}
	}
	return ret_val;
}

void HAL_I2C_MspInit(I2C_HandleTypeDef* i2cHandle)
{

  GPIO_InitTypeDef GPIO_InitStruct;
  if(i2cHandle->Instance==I2C1)
  {
  /* USER CODE BEGIN I2C1_MspInit 0 */

  /* USER CODE END I2C1_MspInit 0 */

    /**I2C1 GPIO Configuration
    PB6     ------> I2C1_SCL
    PB7     ------> I2C1_SDA
    */
    GPIO_InitStruct.Pin = I2C1_SCL_Pin|I2C1_SDA_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_OD;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.Alternate = GPIO_AF4_I2C1;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    /* I2C1 clock enable */
    __HAL_RCC_I2C1_CLK_ENABLE();
  /* USER CODE BEGIN I2C1_MspInit 1 */

  /* USER CODE END I2C1_MspInit 1 */
  }
  else if(i2cHandle->Instance==I2C2)
  {
  /* USER CODE BEGIN I2C2_MspInit 0 */

  /* USER CODE END I2C2_MspInit 0 */

    /**I2C2 GPIO Configuration
    PA9     ------> I2C2_SCL
    PA10     ------> I2C2_SDA
    */
    GPIO_InitStruct.Pin = GPIO_PIN_9|GPIO_PIN_10;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_OD;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF4_I2C2;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    /* I2C2 clock enable */
    __HAL_RCC_I2C2_CLK_ENABLE();
  /* USER CODE BEGIN I2C2_MspInit 1 */

  /* USER CODE END I2C2_MspInit 1 */
  }
}