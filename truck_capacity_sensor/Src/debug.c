/*!
 * debug.c
 *
 *  Created on: Feb 20, 2018
 *      Author: Gabriel
 */

#include "stdint.h"

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

	while((ret_val = CDC_Transmit_FS((uint8_t *)clr_screen_buf, strlen(clr_screen_buf))) != 0) {

	}
	while((ret_val = CDC_Transmit_FS((uint8_t *)Buf, strlen(Buf))) != 0) {}
}

void print_debug_message(char * message) {
	int ret_val = 0;
	while((ret_val = CDC_Transmit_FS((uint8_t *)message, strlen(message))) != 0);
}
