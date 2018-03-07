/*
 * max_i2cxl.h
 *
 *  Created on: Feb 23, 2018
 *      Author: Gabriel
 */

#ifndef MAX_I2CXL_H_
#define MAX_I2CXL_H_

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

#define NUMBER_OF_SENSORS	4

#define SENSOR_1_Pin GPIO_PIN_1
#define SENSOR_2_Pin GPIO_PIN_2
#define SENSOR_3_Pin GPIO_PIN_3
#define SENSOR_4_Pin GPIO_PIN_4
#define SENSOR_5_Pin GPIO_PIN_5
#define SENSOR_6_Pin GPIO_PIN_6
#define SENSOR_7_Pin GPIO_PIN_7
#define SENSOR_8_Pin GPIO_PIN_8

#define SENSOR_1_Port GPIOD
#define SENSOR_2_Port GPIOD
#define SENSOR_3_Port GPIOD
#define SENSOR_4_Port GPIOD
#define SENSOR_5_Port GPIOD
#define SENSOR_6_Port GPIOD
#define SENSOR_7_Port GPIOD
#define SENSOR_8_Port GPIOD

typedef volatile struct max_i2cxl {
	uint8_t addr;
	uint8_t status;
	uint32_t status_pin;
	uint32_t * status_port;
	int last_measurement;
} max_i2cxl_t;

int i2cxl_maxsonar_init(void);
int32_t i2cxl_maxsonar_change_addr(int, int *);
uint32_t i2cxl_maxsonar_start(volatile int, volatile int *);

#endif /* MAX_I2CXL_H_ */