/*
 * ft800_interface.c
 *
 *  Created on: Mar 7, 2018
 *      Author: Gabriel
 */

/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  ** This notice applies to any and all portions of this file
  * that are not between comment pairs USER CODE BEGIN and
  * USER CODE END. Other portions of this file, whether
  * inserted by the user or by software development tools
  * are owned by their respective copyright owners.
  *
  * COPYRIGHT(c) 2018 STMicroelectronics
  *
  * Redistribution and use in source and binary forms, with or without modification,
  * are permitted provided that the following conditions are met:
  *   1. Redistributions of source code must retain the above copyright notice,
  *      this list of conditions and the following disclaimer.
  *   2. Redistributions in binary form must reproduce the above copyright notice,
  *      this list of conditions and the following disclaimer in the documentation
  *      and/or other materials provided with the distribution.
  *   3. Neither the name of STMicroelectronics nor the names of its contributors
  *      may be used to endorse or promote products derived from this software
  *      without specific prior written permission.
  *
  * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
  * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
  * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
  * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
  * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
  * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
  * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
  * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
  * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
  * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
  *
  ******************************************************************************
  */
/* Includes ------------------------------------------------------------------*/
#include <stdint.h>
#include <string.h>
#include "main.h"
#include "ft800.h"
#include "stm32f3xx_hal.h"

/* USER CODE BEGIN Includes */

#define FT800_PD_N	GPIO_PIN_6
#define FT800_CS_N	GPIO_PIN_8

// AN312 FT800 screen size - uncomment the appropriate size
//#define LCD_QVGA																			// VM800B/C 3.5" QVGA - 320x240
#define LCD_WQVGA																				// VM800B/C 4.3" & 5.0" WQVGA - 480x272

// AN_312 Colors - fully saturated colors defined here
#define RED					0xFF0000UL													// Red
#define GREEN				0x00FF00UL													// Green
#define BLUE				0x0000FFUL													// Blue
#define WHITE				0xFFFFFFUL													// White
#define BLACK				0x000000UL

/* USER CODE END Includes */

/* Private variables ---------------------------------------------------------*/
SPI_HandleTypeDef hspi3;
extern volatile int range_cm;

/* USER CODE BEGIN PV */
/* Private variables ---------------------------------------------------------*/

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_SPI3_Init(void);

/* USER CODE BEGIN PFP */
/* Private function prototypes -----------------------------------------------*/

// AN_312 Specific function prototypes - See full descriptions below
static void ft800cmdWrite(unsigned char);
static void ft800memWrite8(unsigned long, unsigned char);
static void ft800memWrite16(unsigned long, unsigned int);
static void ft800memWrite32(unsigned long, unsigned long);
unsigned char ft800memRead8(unsigned long);
unsigned int ft800memRead16(unsigned long);
unsigned long ft800memRead32(unsigned long);
unsigned int incCMDOffset(unsigned int, unsigned char);

void draw_screen(void);
void initial_display_screen(void);
void set_display_parameters(void);
void wake_up_display(void);

//command function prototypes
void cmd_text(int16_t x, int16_t y, int16_t font, uint16_t options, char * s);

//other
int16_t rescale (int16_t old_min, int16_t old_max, int16_t new_min, int16_t new_max, int16_t val);



// AN_312 FT800 pins
unsigned int ft800irqPin;																// Interrupt from FT800 to Arduino - not used here
unsigned int ft800pwrPin;																// PD_N from Arduino to FT800 - effectively FT800 reset
unsigned int ft800csPin;																// SPI chip select - defined separately since it's manipulated with GPIO calls

// AN_312 LCD display parameters
unsigned int lcdWidth;																	// Active width of LCD display
unsigned int lcdHeight;																	// Active height of LCD display
unsigned int lcdHcycle;																	// Total number of clocks per line
unsigned int lcdHoffset;																// Start of active line
unsigned int lcdHsync0;																	// Start of horizontal sync pulse
unsigned int lcdHsync1;																	// End of horizontal sync pulse
unsigned int lcdVcycle;																	// Total number of lines per screen
unsigned int lcdVoffset;																// Start of active screen
unsigned int lcdVsync0;																	// Start of vertical sync pulse
unsigned int lcdVsync1;																	// End of vertical sync pulse
unsigned char lcdPclk;																	// Pixel Clock
unsigned char lcdSwizzle;																// Define RGB output pins
unsigned char lcdPclkpol;																// Define active edge of PCLK

unsigned long ramDisplayList = RAM_DL;									// Set beginning of display list memory
unsigned long ramCommandBuffer = RAM_CMD;								// Set beginning of graphics command memory

unsigned int cmdBufferRd = 0x0000;											// Used to navigate command ring buffer
unsigned int cmdBufferWr = 0x0000;											// Used to navigate command ring buffer
unsigned int cmdOffset = 0x0000;												// Used to navigate command rung buffer
unsigned int point_size = 0x0100;												// Define a default dot size
unsigned long point_x = (96 * 16);											// Define a default point x-location (1/16 anti-aliased)
unsigned long point_y = (136 * 16);											// Define a default point y-location (1/16 anti-aliased)
unsigned long color;																		// Variable for chanign colors
unsigned char ft800Gpio;																// Used for FT800 GPIO register

//range finder variables
int16_t sensor_range1;
int16_t sensor_range2;
int16_t sensor_range3;
int16_t sensor_range4;

int16_t midx;
int16_t midy;

typedef struct {
	int16_t xpos;
	int16_t xsize;
	int16_t ypos;
	int16_t ysize;
	int16_t total_range_cm;
	int16_t *range_value;
} gauge_t;

void drawGauge(gauge_t * gauge){

	ft800memWrite32(RAM_CMD + cmdOffset, (DL_BEGIN | RECTS)); //start rectangles
	cmdOffset = incCMDOffset(cmdOffset, 4);

	ft800memWrite32(RAM_CMD + cmdOffset, (DL_COLOR_RGB | RED));
	cmdOffset = incCMDOffset(cmdOffset, 4);

	ft800memWrite32(RAM_CMD + cmdOffset, (DL_LINE_WIDTH | 10*16));
	cmdOffset = incCMDOffset(cmdOffset, 4);

	ft800memWrite32(RAM_CMD + cmdOffset, (DL_VERTEX2F | ((gauge->xpos)*16 << 15) | ((gauge->ypos)*16)));  //draw background of gauge
	cmdOffset = incCMDOffset(cmdOffset, 4);

	ft800memWrite32(RAM_CMD + cmdOffset, (DL_VERTEX2F | ((gauge->xpos+gauge->xsize))*16 << 15) | ((gauge->ypos-gauge->ysize)*16));  //draw background of gauge
	cmdOffset = incCMDOffset(cmdOffset, 4);

	ft800memWrite32(RAM_CMD + cmdOffset, (DL_COLOR_RGB | BLACK));
	cmdOffset = incCMDOffset(cmdOffset, 4);

	ft800memWrite32(RAM_CMD + cmdOffset, (DL_LINE_WIDTH | 1*16));
	cmdOffset = incCMDOffset(cmdOffset, 4);

	ft800memWrite32(RAM_CMD + cmdOffset, (DL_VERTEX2F | ((gauge->xpos)*16 << 15) | ((gauge->ypos)*16)));
	cmdOffset = incCMDOffset(cmdOffset, 4);

	ft800memWrite32(RAM_CMD + cmdOffset, (DL_VERTEX2F | ((gauge->xpos+gauge->xsize)*16 << 15) | ((gauge->ypos-gauge->ysize)*16)));
	cmdOffset = incCMDOffset(cmdOffset, 4);

	ft800memWrite32(RAM_CMD + cmdOffset, (DL_COLOR_RGB | WHITE));
	cmdOffset = incCMDOffset(cmdOffset, 4);

	int16_t length;
	length = rescale(0,gauge->total_range_cm,0,gauge->ysize,*(gauge->range_value));

	ft800memWrite32(RAM_CMD + cmdOffset, (DL_VERTEX2F | ((gauge->xpos)*16 << 15) | ((gauge->ypos)*16)));
	cmdOffset = incCMDOffset(cmdOffset, 4);

	ft800memWrite32(RAM_CMD + cmdOffset, (DL_VERTEX2F | ((gauge->xpos+gauge->xsize)*16 << 15) | ((gauge->ypos-length)*16)));
	cmdOffset = incCMDOffset(cmdOffset, 4);

	ft800memWrite32(RAM_CMD + cmdOffset, (DL_END));				// End RECT
	cmdOffset = incCMDOffset(cmdOffset, 4);

	char length_txt[4];
	sprintf(length_txt,"%03d",*(gauge->range_value));
	cmd_text(gauge->xpos+(gauge->xsize/2),gauge->ypos+20,26,OPT_CENTER,length_txt);

}


/* USER CODE END PFP */

/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  *
  * @retval None
  */
//int main(void)
//{
//  /* USER CODE BEGIN 1 */
//
//  /* USER CODE END 1 */
//
//  /* MCU Configuration----------------------------------------------------------*/
//
//  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
//  HAL_Init();
//
//  /* USER CODE BEGIN Init */
//
//  /* USER CODE END Init */
//
//  /* Configure the system clock */
//  SystemClock_Config();
//
//  /* USER CODE BEGIN SysInit */
//
//  /* USER CODE END SysInit */
//
//  /* Initialize all configured peripherals */
//  MX_GPIO_Init();
//  MX_SPI3_Init();
//
//  set_display_parameters();
//  wake_up_display();
//  initial_display_screen();
//  /* USER CODE BEGIN 2 */
//
//	//HAL_Delay(2000);
//
//
//  // End of Write Initial Display List & Enable Display
//  //***************************************
//
//  /* USER CODE END 2 */
//
//  /* Infinite loop */
//  /* USER CODE BEGIN WHILE */
//	while (1)
//	{
//
//		draw_screen();
//		HAL_Delay(200);
//
//  /* USER CODE END WHILE */
//
//  /* USER CODE BEGIN 3 */
//
//	}
//  /* USER CODE END 3 */
//
//}

/**
  * @brief System Clock Configuration
  * @retval None
  */
//void SystemClock_Config(void)
//{
//
//  RCC_OscInitTypeDef RCC_OscInitStruct;
//  RCC_ClkInitTypeDef RCC_ClkInitStruct;
//
//    /**Initializes the CPU, AHB and APB busses clocks
//    */
//  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
//  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
//  RCC_OscInitStruct.HSICalibrationValue = 16;
//  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
//  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
//  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL12;
//  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
//  {
//    _Error_Handler(__FILE__, __LINE__);
//  }
//
//    /**Initializes the CPU, AHB and APB busses clocks
//    */
//  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
//                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
//  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
//  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
//  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
//  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;
//
//  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_1) != HAL_OK)
//  {
//    _Error_Handler(__FILE__, __LINE__);
//  }
//
//    /**Configure the Systick interrupt time
//    */
//  HAL_SYSTICK_Config(HAL_RCC_GetHCLKFreq()/1000);
//
//    /**Configure the Systick
//    */
//  HAL_SYSTICK_CLKSourceConfig(SYSTICK_CLKSOURCE_HCLK);
//
//  /* SysTick_IRQn interrupt configuration */
//  HAL_NVIC_SetPriority(SysTick_IRQn, 0, 0);
//}


/* USER CODE BEGIN 4 */

/******************************************************************************
 * Function:        void ft800cmdWrite(ftCommand)
 * PreCondition:    None
 * Input:           ftCommand
 * Output:          None
 * Side Effects:    None
 * Overview:        Sends FT800 command
 * Note:            None
 *****************************************************************************/
void ft800cmdWrite(unsigned char ftCommand)
{
	unsigned char cZero = 0x00;														// Filler value for command
	HAL_GPIO_WritePin(GPIOA, FT800_CS_N, GPIO_PIN_RESET);	// Set chip select low
	HAL_SPI_Transmit(&hspi3, &ftCommand, 1, 0);						// Send command
	HAL_SPI_Transmit(&hspi3, &cZero, 1, 0);								// Send first filler byte
	HAL_SPI_Transmit(&hspi3, &cZero, 1, 0);								// Send second filler byte
	HAL_GPIO_WritePin(GPIOA, FT800_CS_N, GPIO_PIN_SET);		// Set chip select high
}

/******************************************************************************
 * Function:        void ft800memWritexx(ftAddress, ftDataxx, ftLength)
 * PreCondition:    None
 * Input:           ftAddress = FT800 memory space address
 *                  ftDataxx = a byte, int or long to send
 * Output:          None
 * Side Effects:    None
 * Overview:        Writes FT800 internal address space
 * Note:            "xx" is one of 8, 16 or 32
 *****************************************************************************/
void ft800memWrite8(unsigned long ftAddress, unsigned char ftData8)
{
	unsigned char cTempAddr[3];														// FT800 Memory Address

	cTempAddr[2] = (char) (ftAddress >> 16) | MEM_WRITE;	// Compose the command and address to send
	cTempAddr[1] = (char) (ftAddress >> 8);								// middle byte
	cTempAddr[0] = (char) (ftAddress);										// low byte

	HAL_GPIO_WritePin(GPIOA, FT800_CS_N, GPIO_PIN_RESET);	// Set chip select low

  for (int i = 2; i >= 0; i--)
	{
		HAL_SPI_Transmit(&hspi3, &cTempAddr[i], 1, 0); 			// Send Memory Write plus high address byte
	}

	for (int j = 0; j < sizeof(ftData8); j++)
	{
		HAL_SPI_Transmit(&hspi3, &ftData8, 1, 0);						// Send data byte
	}

	HAL_GPIO_WritePin(GPIOA, FT800_CS_N, GPIO_PIN_SET);		// Set chip select high
}

void ft800memWrite16(unsigned long ftAddress, unsigned int ftData16)
{
	unsigned char cTempAddr[3];														// FT800 Memory Address
	unsigned char cTempData[2];														// 16-bit data to write

	cTempAddr[2] = (char) (ftAddress >> 16) | MEM_WRITE;	// Compose the command and address to send
	cTempAddr[1] = (char) (ftAddress >> 8);								// middle byte
	cTempAddr[0] = (char) (ftAddress);										// low byte

	cTempData[1] = (char) (ftData16 >> 8);								// Compose data to be sent - high byte
	cTempData[0] = (char) (ftData16);											// low byte

	HAL_GPIO_WritePin(GPIOA, FT800_CS_N, GPIO_PIN_RESET);	// Set chip select low

  for (int i = 2; i >= 0; i--)
	{
		HAL_SPI_Transmit(&hspi3, &cTempAddr[i], 1, 0); 			// Send Memory Write plus high address byte
	}

	for (int j = 0; j < sizeof(cTempData); j++)						// Start with least significant byte
	{
		HAL_SPI_Transmit(&hspi3, &cTempData[j], 1, 0);			// Send data byte
	}

	HAL_GPIO_WritePin(GPIOA, FT800_CS_N, GPIO_PIN_SET);		// Set chip select high
}

void ft800memWrite32(unsigned long ftAddress, unsigned long ftData32)
{
	unsigned char cTempAddr[3];														// FT800 Memory Address
	unsigned char cTempData[4];														// 32-bit data to write

	cTempAddr[2] = (char) (ftAddress >> 16) | MEM_WRITE;	// Compose the command and address to send
	cTempAddr[1] = (char) (ftAddress >> 8);								// middle byte
	cTempAddr[0] = (char) (ftAddress);										// low byte

	cTempData[3] = (char) (ftData32 >> 24);								// Compose data to be sent - high byte
	cTempData[2] = (char) (ftData32 >> 16);								//
	cTempData[1] = (char) (ftData32 >> 8);								//
	cTempData[0] = (char) (ftData32);											// low byte

	HAL_GPIO_WritePin(GPIOA, FT800_CS_N, GPIO_PIN_RESET);	// Set chip select low

  for (int i = 2; i >= 0; i--)
	{
		HAL_SPI_Transmit(&hspi3, &cTempAddr[i], 1, 0); 			// Send Memory Write plus high address byte
	}

	for (int j = 0; j < sizeof(cTempData); j++)						// Start with least significant byte
	{
		HAL_SPI_Transmit(&hspi3, &cTempData[j], 1, 0);			// Send SPI byte
	}

	HAL_GPIO_WritePin(GPIOA, FT800_CS_N, GPIO_PIN_SET);		// Set chip select high
}
/******************************************************************************
 * Function:        unsigned char ft800memReadxx(ftAddress, ftLength)
 * PreCondition:    None
 * Input:           ftAddress = FT800 memory space address
 * Output:          ftDataxx (byte, int or long)
 * Side Effects:    None
 * Overview:        Reads FT800 internal address space
 * Note:            "xx" is one of 8, 16 or 32
 *****************************************************************************/
unsigned char ft800memRead8(unsigned long ftAddress)
{
  unsigned char ftData8 = ZERO;													// Place-holder for 8-bits being read
	unsigned char cTempAddr[3];														// FT800 Memory Address
	unsigned char cZeroFill = ZERO;												// Dummy byte

	cTempAddr[2] = (char) (ftAddress >> 16) | MEM_READ;		// Compose the command and address to send
	cTempAddr[1] = (char) (ftAddress >> 8);								// middle byte
	cTempAddr[0] = (char) (ftAddress);										// low byte

	HAL_GPIO_WritePin(GPIOA, FT800_CS_N, GPIO_PIN_RESET);	// Set chip select low

  for (int i = 2; i >= 0; i--)
	{
		HAL_SPI_Transmit(&hspi3, &cTempAddr[i], 1, 0); 			// Send Memory Write plus high address byte
	}

  HAL_SPI_Transmit(&hspi3, &cZeroFill, 1, 0);						// Send dummy byte
	for (int j = 0; j < sizeof(ftData8); j++)							// Start with least significant byte
	{
		HAL_SPI_Receive(&hspi3, &ftData8, 1, 0);						// Receive data byte
	}

	HAL_GPIO_WritePin(GPIOA, FT800_CS_N, GPIO_PIN_SET);		// Set chip select high

  return ftData8;																				// Return 8-bits
}

unsigned int ft800memRead16(unsigned long ftAddress)
{
  unsigned int ftData16;																// 16-bits to return
	unsigned char cTempAddr[3];														// FT800 Memory Address
	unsigned char cTempData[2];														// Place-holder for 16-bits being read
	unsigned char cZeroFill;

	cTempAddr[2] = (char) (ftAddress >> 16) | MEM_READ;		// Compose the command and address to send
	cTempAddr[1] = (char) (ftAddress >> 8);								// middle byte
	cTempAddr[0] = (char) (ftAddress);										// low byte

	HAL_GPIO_WritePin(GPIOA, FT800_CS_N, GPIO_PIN_RESET);	// Set chip select low

  for (int i = 2; i >= 0; i--)
	{
		HAL_SPI_Transmit(&hspi3, &cTempAddr[i], 1, 0); 			// Send Memory Write plus high address byte
	}

  HAL_SPI_Transmit(&hspi3, &cZeroFill, 1, 0);						// Send dummy byte

	for (int j = 0; j < sizeof(cTempData); j++)						// Start with least significant byte
	{
		HAL_SPI_Receive(&hspi3, &cTempData[j], 1, 0);				// Receive data byte
	}

	HAL_GPIO_WritePin(GPIOA, FT800_CS_N, GPIO_PIN_SET);		// Set chip select high

	ftData16 = (cTempData[1]<< 8) | 											// Compose value to return - high byte
					   (cTempData[0]); 														// low byte

  return ftData16;																			// Return 16-bits
}

unsigned long ft800memRead32(unsigned long ftAddress)
{
  unsigned long ftData32;																// 32-bits to return
	unsigned char cTempAddr[3];														// FT800 Memory Address
	unsigned char cTempData[4];														// Place holder for 32-bits being read
	unsigned char cZeroFill;															// Dummy byte

	cTempAddr[2] = (char) (ftAddress >> 16) | MEM_READ;		// Compose the command and address to send
	cTempAddr[1] = (char) (ftAddress >> 8);								// middle byte
	cTempAddr[0] = (char) (ftAddress);										// low byte

	HAL_GPIO_WritePin(GPIOA, FT800_CS_N, GPIO_PIN_RESET);	// Set chip select low

  for (int i = 2; i >= 0; i--)
	{
		HAL_SPI_Transmit(&hspi3, &cTempAddr[i], 1, 0); 			// Send Memory Write plus high address byte
	}

  HAL_SPI_Transmit(&hspi3, &cZeroFill, 1, 0);						// Send dummy byte

	for (int j = 0; j < sizeof(cTempData); j++)						// Start with least significatn byte
	{
		HAL_SPI_Receive(&hspi3, &cTempData[j], 1, 0);				// Receive data byte
	}

	HAL_GPIO_WritePin(GPIOA, FT800_CS_N, GPIO_PIN_SET);		// Set chip select high

	ftData32 = (cTempData[3]<< 24) | 											// Compose value to return - high byte
						 (cTempData[2]<< 16) |
						 (cTempData[1]<< 8) |
						 (cTempData[0]); 														// Low byte

  return ftData32;																			// Return 32-bits
}

/******************************************************************************
 * Function:        void incCMDOffset(currentOffset, commandSize)
 * PreCondition:    None
 *                    starting a command list
 * Input:           currentOffset = graphics processor command list pointer
 *                  commandSize = number of bytes to increment the offset
 * Output:          newOffset = the new ring buffer pointer after adding the command
 * Side Effects:    None
 * Overview:        Adds commandSize to the currentOffset.
 *                  Checks for 4K ring-buffer offset roll-over
 * Note:            None
 *****************************************************************************/
unsigned int incCMDOffset(unsigned int currentOffset, unsigned char commandSize)
{
    unsigned int newOffset;															// Used to hold new offset
    newOffset = currentOffset + commandSize;						// Calculate new offset
    if(newOffset > 4095)																// If new offset past boundary...
    {
        newOffset = (newOffset - 4096);									// ... roll over pointer
    }
    return newOffset;																		// Return new offset
}

int16_t rescale (int16_t old_min, int16_t old_max, int16_t new_min, int16_t new_max, int16_t val){
	float a = new_max-new_min;
	float b = old_max-old_min;
	float c = val - old_min;
	return  (int) (((a*c)/(b)) + old_min);
}

void cmd_text(int16_t x, int16_t y, int16_t font, uint16_t options, char * s){

	ft800memWrite32(RAM_CMD + cmdOffset, (CMD_TEXT)); //write text command
	cmdOffset = incCMDOffset(cmdOffset, 4);

	ft800memWrite16(RAM_CMD + cmdOffset, x); //write X pos
	cmdOffset = incCMDOffset(cmdOffset, 2); //inc 2 bytes

	ft800memWrite16(RAM_CMD + cmdOffset, y); //write Y pos
	cmdOffset = incCMDOffset(cmdOffset, 2); //inc 2 bytes

	ft800memWrite16(RAM_CMD + cmdOffset, font); //write FONT
	cmdOffset = incCMDOffset(cmdOffset, 2); //inc 2 bytes

	ft800memWrite16(RAM_CMD + cmdOffset, options); //write options
	cmdOffset = incCMDOffset(cmdOffset, 2); //inc 2 bytes
	int i = 0;
	while (s[i] != '\0'){ //write the string
		ft800memWrite8(RAM_CMD + cmdOffset, s[i]);
		cmdOffset = incCMDOffset(cmdOffset, 1);
		i++;
	}
	uint8_t num_null = 4 - (strlen(s) % 4); //must make sure to write 4 byte commands, pad with null terminators
	for (i = 0; i < num_null; i++){
		ft800memWrite8(RAM_CMD + cmdOffset, 0); //null terminate
		cmdOffset = incCMDOffset(cmdOffset, 1);
	}

}

void draw_screen(void){
	// Wait for graphics processor to complete executing the current command list
	// This happens when REG_CMD_READ matches REG_CMD_WRITE, indicating that all commands
	// have been executed.  The next commands get executed when REG_CMD_WRITE is updated again...
	// then REG_CMD_READ again catches up to REG_CMD_WRITE
	// This is a 4Kbyte ring buffer, so keep pointers within the 4K roll-over
	do
	{
		cmdBufferRd = ft800memRead16(REG_CMD_READ);					// Read the graphics processor read pointer
		cmdBufferWr = ft800memRead16(REG_CMD_WRITE); 				// Read the graphics processor write pointer
	}while (cmdBufferWr != cmdBufferRd);									// Wait until the two registers match

	cmdOffset = cmdBufferWr;															// The new starting point the first location after the last command
																								// Otherwise...
	color = WHITE;																				// change the color to red

	ft800memWrite32(RAM_CMD + cmdOffset, (CMD_DLSTART));// Start the display list
	cmdOffset = incCMDOffset(cmdOffset, 4);								// Update the command pointer

	ft800memWrite32(RAM_CMD + cmdOffset, (DL_CLEAR_RGB | BLACK));
																											// Set the default clear color to black
	cmdOffset = incCMDOffset(cmdOffset, 4);								// Update the command pointer


	ft800memWrite32(RAM_CMD + cmdOffset, (DL_CLEAR | CLR_COL | CLR_STN | CLR_TAG));
																											// Clear the screen - this and the previous prevent artifacts between lists
																											// Attributes are the color, stencil and tag buffers
	cmdOffset = incCMDOffset(cmdOffset, 4);								// Update the command pointer

	gauge_t test1 = {midx-100,25,midy+80,160,765, &range_cm};
//	gauge_t test2 = {midx-50,25,midy+80,160,765,&sensor_range2};
//	gauge_t test3 = {midx,25,midy+80,160,765,&sensor_range3};
//	gauge_t test4 = {midx+50,25,midy+80,160,765,&sensor_range4};

	drawGauge(&test1);
//	drawGauge(&test2);
//	drawGauge(&test3);
//	drawGauge(&test4);

	ft800memWrite32(RAM_CMD + cmdOffset, (DL_DISPLAY));		// Instruct the graphics processor to show the list
	cmdOffset = incCMDOffset(cmdOffset, 4);								// Update the command pointer

	ft800memWrite32(RAM_CMD + cmdOffset, (CMD_SWAP));			// Make this list active
	cmdOffset = incCMDOffset(cmdOffset, 4);								// Update the command pointer


	ft800memWrite16(REG_CMD_WRITE, (cmdOffset));					// Update the ring buffer pointer so the graphics processor starts executing

	sensor_range1 += 22;
	if (sensor_range1 > 765) sensor_range1 = 0;
	sensor_range2 += 33;
	if (sensor_range2 > 765) sensor_range2 = 0;
	sensor_range3 += 44;
	if (sensor_range3 > 765) sensor_range3 = 0;
	sensor_range4 += 55;
	if (sensor_range4 > 765) sensor_range4 = 0;

}

void initial_display_screen(void){
	//***************************************
	// Write Initial Display List & Enable Display

	ramDisplayList = RAM_DL;															// start of Display List
	ft800memWrite32(ramDisplayList, DL_CLEAR_RGB); 				// Clear Color RGB   00000010 RRRRRRRR GGGGGGGG BBBBBBBB  (R/G/B = Colour values) default zero / black
	ramDisplayList += 4;																	// point to next location
	ft800memWrite32(ramDisplayList, (DL_CLEAR | CLR_COL | CLR_STN | CLR_TAG));
																												// Clear 00100110 -------- -------- -----CST  (C/S/T define which parameters to clear)
	ramDisplayList += 4;																	// point to next location
	ft800memWrite32(ramDisplayList, DL_DISPLAY);					// DISPLAY command 00000000 00000000 00000000 00000000 (end of display list)

	ft800memWrite32(REG_DLSWAP, DLSWAP_FRAME);						// 00000000 00000000 00000000 000000SS  (SS bits define when render occurs)
																												// Nothing is being displayed yet... the pixel clock is still 0x00
	ramDisplayList = RAM_DL;															// Reset Display List pointer for next list

	ft800Gpio = ft800memRead8(REG_GPIO);									// Read the FT800 GPIO register for a read/modify/write operation
	ft800Gpio = ft800Gpio | 0x80;													// set bit 7 of FT800 GPIO register (DISP) - others are inputs
	ft800memWrite8(REG_GPIO, ft800Gpio);									// Enable the DISP signal to the LCD panel
	ft800memWrite8(REG_PCLK, lcdPclk);										// Now start clocking data to the LCD panel
	for(int duty = 0; duty <= 128; duty++)
	{
	  ft800memWrite8(REG_PWM_DUTY, duty);									// Turn on backlight - ramp up slowly to full brighness
	  HAL_Delay(10);
	}
}

void set_display_parameters(void){
	lcdWidth   = 480;																			// Active width of LCD display
	lcdHeight  = 272;																			// Active height of LCD display
	lcdHcycle  = 548;																			// Total number of clocks per line
	lcdHoffset = 43;																			// Start of active line
	lcdHsync0  = 0;																				// Start of horizontal sync pulse
	lcdHsync1  = 41;																			// End of horizontal sync pulse
	lcdVcycle  = 292;																			// Total number of lines per screen
	lcdVoffset = 12;																			// Start of active screen
	lcdVsync0  = 0;																				// Start of vertical sync pulse
	lcdVsync1  = 10;																			// End of vertical sync pulse
	lcdPclk    = 5;																				// Pixel Clock
	lcdSwizzle = 0;																				// Define RGB output pins
	lcdPclkpol = 1;																				// Define active edge of PCLK

	midx = lcdWidth/2;
	midy = lcdHeight/2;
}

void wake_up_display(void){


	HAL_GPIO_WritePin(GPIOC, FT800_PD_N, GPIO_PIN_SET); 	// Initial state of PD_N - high
	HAL_GPIO_WritePin(GPIOA, FT800_CS_N, GPIO_PIN_SET);		// Initial state of SPI CS - high
	HAL_Delay(20);																				// Wait 20ms
	HAL_GPIO_WritePin(GPIOC, FT800_PD_N, GPIO_PIN_RESET); // Reset FT800
	HAL_Delay(20);																				// Wait 20ms
	HAL_GPIO_WritePin(GPIOC, FT800_PD_N, GPIO_PIN_SET); 	// FT800 is awake
	HAL_Delay(20);																				// Wait 20ms - required

	ft800cmdWrite(FT800_ACTIVE);													// Start FT800
	HAL_Delay(5);																					// Give some time to process
	ft800cmdWrite(FT800_CLKEXT);													// Set FT800 for external clock
	HAL_Delay(5);																					// Give some time to process
	ft800cmdWrite(FT800_CLK48M);													// Set FT800 for 48MHz PLL
	HAL_Delay(5);																					// Give some time to process
							// Now FT800 can accept commands at up to 30MHz clock on SPI bus
							//   This application leaves the SPI bus at 8MHz
	///////////////////////////////
	//Getting stuck in the while loop here
	//////////////////////////////////
	unsigned char rr;
	while ((rr = ft800memRead8(REG_ID)) != 0x7C)											// Read ID register - is it 0x7C?
	{
	  //while(1);	// If we don't get 0x7C, the ineface isn't working - halt with infinite loop
	}

	ft800memWrite8(REG_PCLK, ZERO);												// Set PCLK to zero - don't clock the LCD until later
	ft800memWrite8(REG_PWM_DUTY, ZERO);										// Turn off backlight

	// End of Wake-up FT800

	//***************************************
	// Configure Touch and Audio - not used in this example, so disable both
	ft800memWrite8(REG_TOUCH_MODE, ZERO);									// Disable touch
	ft800memWrite16(REG_TOUCH_RZTHRESH, ZERO);						// Eliminate any false touches

	ft800memWrite8(REG_VOL_PB, ZERO);											// turn recorded audio volume down
	ft800memWrite8(REG_VOL_SOUND, ZERO);									// turn synthesizer volume down
	ft800memWrite16(REG_SOUND, 0x6000);										// set synthesizer to mute

	// End of Configure Touch and Audio
	//***************************************
}


/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @param  file: The file name as string.
  * @param  line: The line in file as a number.
  * @retval None
  */
//void _Error_Handler(char *file, int line)
//{
//  /* USER CODE BEGIN Error_Handler_Debug */
//  /* User can add his own implementation to report the HAL error return state */
//  while(1)
//  {
//  }
//  /* USER CODE END Error_Handler_Debug */
//}

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
     tex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
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
