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

#define MEAS_BUF_SIZE		256
#define CLR_SCR_BUF_SIZE	6

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
