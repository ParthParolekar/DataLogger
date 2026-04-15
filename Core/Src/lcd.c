/*
 * lcd.c
 *
 *  Created on: 09-Apr-2026
 *      Author: Parth
 */


#include "lcd.h"
#include <string.h>

//Private Macros
#define LCD_BACKLIGHT			0x08		//P3 bit
#define LCD_NO_BACKLIGHT		0x00

#define LCD_EN					0x04		//P2 bit - Enable
#define LCD_RW					0x02		//P1 bit - Read/Write (always 0)
#define LCD_RS					0x01		//P0 bit - Register Select

//Commands
#define LCD_CLEAR_DISPLAY		0x01
#define LCD_RETURN_HOME			0x02
#define LCD_ENTRY_MODE			0x06
#define LCD_DISPLAY_ON			0x0C		//Display ON, Cursor OFF, Blink OFF
#define LCD_FUNCTION_SET		0x28		//4 bit, 2 lines, 5x8 font
#define LCD_SET_DDRAM			0x80		//Base Address for DDRAM

//Row Offset
#define LCD_ROW0_OFFSET			0x00
#define LCD_ROW1_OFFSET			0x40

//Private Variables
static I2C_HandleTypeDef *_hi2c;
static uint8_t _backlight = LCD_BACKLIGHT;	//On by default

//Private Function Prototypes
static void LCD_SendByte(uint8_t byte, uint8_t rs);
static void LCD_SendNibble(uint8_t nibble);
static void LCD_I2C_Write(uint8_t data);
static void	LCD_SendCommand(uint8_t cmd);
static void LCD_SendData(uint8_t data);

//Public Functions
LCD_Status_t LCD_Init(I2C_HandleTypeDef *hi2c){
	_hi2c = hi2c;

	HAL_Delay(100);

	//1) Init sequence - force 4 bit mode
	LCD_SendNibble(0x03);
	HAL_Delay(10);
	LCD_SendNibble(0x03);
	HAL_Delay(5);
	LCD_SendNibble(0x03);
	HAL_Delay(2);

	//2) Switch to 4 bit mode
	LCD_SendNibble(0x02);
	HAL_Delay(2);

	LCD_SendCommand(LCD_FUNCTION_SET); 	//Configure: 4 Bit, 2 Lines, 5x8 Font
	HAL_Delay(2);

	LCD_SendCommand(LCD_DISPLAY_ON);	//Display ON, Cursor OFF
	HAL_Delay(2);

	LCD_SendCommand(LCD_CLEAR_DISPLAY);	//Clear Display
	HAL_Delay(5);						//Clear needs more time

	LCD_SendCommand(LCD_ENTRY_MODE);	//Move cursor to right
	HAL_Delay(2);
	return LCD_OK;
}

void LCD_Clear(void){
	LCD_SendCommand(LCD_CLEAR_DISPLAY);
	HAL_Delay(2);
}

void LCD_SetCursor(uint8_t row, uint8_t col){
	uint8_t offsets[] = {LCD_ROW0_OFFSET, LCD_ROW1_OFFSET};

	if(row > LCD_ROWS) row = LCD_ROWS - 1;
	if(col > LCD_COLS) col = LCD_COLS - 1;

	LCD_SendCommand(LCD_SET_DDRAM | (offsets[row] + col));
}

void LCD_Print(char *str){
	while(*str){
	LCD_PrintChar(*str++);
	}
}

void LCD_PrintChar(char ch){
	LCD_SendData((uint8_t)ch);
}

void LCD_Backlight(uint8_t on){
	_backlight = on ? LCD_BACKLIGHT : LCD_NO_BACKLIGHT;
	LCD_I2C_Write(0x00);		//Send dummy data to apply backlight setting immediately
}


//Private Functions
static void LCD_I2C_Write(uint8_t data){
	data |= _backlight;
	HAL_I2C_Master_Transmit(_hi2c, (LCD_I2C_ADDR << 1), &data, 1, HAL_MAX_DELAY);
}

static void LCD_SendNibble(uint8_t nibble){
	uint8_t data = (nibble << 4) & 0xF0;

	LCD_I2C_Write(data | LCD_EN);		//EN High
	HAL_Delay(1);
	LCD_I2C_Write(data & ~(LCD_EN));	//EN Low
	HAL_Delay(1);
}

static void LCD_SendByte(uint8_t byte, uint8_t rs){
	uint8_t rs_bit = rs ? LCD_RS : 0x00;

	uint8_t high = (byte & 0xF0) >> 4;
	uint8_t low = (byte & 0x0F);

	//Send High
	uint8_t data = ((high << 4) & 0XF0) | rs_bit;
	LCD_I2C_Write(data | LCD_EN);
	HAL_Delay(1);
	LCD_I2C_Write(data & ~(LCD_EN));
	HAL_Delay(1);

	//Send Low
	data = ((low << 4) & 0xF0) | rs_bit;
	LCD_I2C_Write(data | LCD_EN);
	HAL_Delay(1);
	LCD_I2C_Write(data & ~(LCD_EN));
	HAL_Delay(1);
}

static void LCD_SendCommand(uint8_t cmd){
	LCD_SendByte(cmd, 0);		//RS = 0 for Command
}

static void LCD_SendData(uint8_t data){
	LCD_SendByte(data, 1);		//RS = 1 for Data
}



