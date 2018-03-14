/*!
 * debug.c
 *
 *  Created on: Feb 20, 2018
 *      Author: Gabriel
 */

#include "main.h"
#ifdef USE_USB_DEBUG
#include "stdint.h"
#include "string.h"
#include "stdio.h"
#include "usbd_cdc_if.h"
#include "max_i2cxl.h"

#define MEAS_BUF_SIZE		256
#define CLR_SCR_BUF_SIZE	8
#define CRSOR_UP_BUF_SIZE	8

extern I2C_HandleTypeDef hi2c2;

void print_measurement_all(max_i2cxl_t * sensor, uint8_t length) {
	char ESC = 27;	//ESC character
	char temp_buf[MEAS_BUF_SIZE];
	char Buf[MEAS_BUF_SIZE];
	char clr_screen_buf[CLR_SCR_BUF_SIZE];
	char crsr_up_buf[CRSOR_UP_BUF_SIZE];
	int ret_val = 0;

	/* Clear buffers */
	memset(crsr_up_buf, 0, CRSOR_UP_BUF_SIZE);
	memset(clr_screen_buf, 0, CLR_SCR_BUF_SIZE);
	memset(Buf, 0, MEAS_BUF_SIZE);
	memset(temp_buf, 0, MEAS_BUF_SIZE);

	/* Make buffers */
	sprintf(crsr_up_buf, "%c[%dA\r", ESC, length);	//cursor up length times
	sprintf(clr_screen_buf, "%c[J\r", ESC);	//clear screen from cursor down

	for (int cnt = 0; cnt < length; cnt++) {
		if(sensor[cnt].error_code == 0) {
			sprintf(temp_buf, "MEASUREMENT: %d cm\r\n", sensor[cnt].last_measurement);
			strcat(Buf, temp_buf);
		}
		else {
			sprintf(temp_buf, "[ERROR] MEASUREMENT ERROR: 0x%08X I2C Error: %d\r\n", sensor[cnt].error_code, HAL_I2C_GetError(&hi2c2));
			strcat(Buf, temp_buf);
		}
	}

	/* Transmit */
	while((ret_val = CDC_Transmit_FS((uint8_t *)clr_screen_buf, strlen(clr_screen_buf))) != 0);
	while((ret_val = CDC_Transmit_FS((uint8_t *)Buf, strlen(Buf))) != 0);
	while((ret_val = CDC_Transmit_FS((uint8_t *)crsr_up_buf, strlen(crsr_up_buf))) != 0);
}

void print_measurement(int measurement) {
	char ESC = 27;	//ESC character
	char Buf[MEAS_BUF_SIZE];
	char clr_screen_buf[CLR_SCR_BUF_SIZE];
	int ret_val = 0;
	memset(Buf, 0, CLR_SCR_BUF_SIZE);
	memset(Buf, 0, MEAS_BUF_SIZE);

	/* Make buffers */
	sprintf(clr_screen_buf, "%c[2K\r", ESC);
	sprintf(Buf, "MEASUREMENT: %d cm", measurement);

	/* Transmit */
	while((ret_val = CDC_Transmit_FS((uint8_t *)clr_screen_buf, strlen(clr_screen_buf))) != 0);
	while((ret_val = CDC_Transmit_FS((uint8_t *)Buf, strlen(Buf))) != 0);
}

void print_debug_message(char * message) {
	int ret_val = 0;
	while((ret_val = CDC_Transmit_FS((uint8_t *)message, strlen(message))) != 0);
}

void print_stamp() {
	int ret_val = 0;
	char stamp[1024] = "---------------------------------------\r\n";
	strcat(stamp, "Truck Capacity Sensor - Group 1\r\n");
	strcat(stamp, "G Yano, J Kapp, M Lang\r\n");
	char compile_buf[MAX_BUF_SIZE];
	sprintf(compile_buf, "Compiled on: %s At: %s\r\n", __DATE__, __TIME__);
	strcat(stamp, compile_buf);
	strcat(stamp, "---------------------------------------\r\n\n");

	while((ret_val = CDC_Transmit_FS((uint8_t *)stamp, strlen(stamp))) != 0);
}
#endif
