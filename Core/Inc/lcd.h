/*
 * lcd.h
 *
 *  Created on: 09-Apr-2026
 *      Author: Parth
 */

#ifndef INC_LCD_H_
#define INC_LCD_H_

#include "main.h"

//Public Macros
#define LCD_I2C_ADDR		0x27
#define LCD_COLS			16
#define LCD_ROWS			2

//Status Code
typedef enum {
	LCD_OK = 0,
	LCD_ERROR
} LCD_Status_t;

//Public APIs
LCD_Status_t LCD_Init(I2C_HandleTypeDef *hi2c);
void LCD_Clear(void);
void LCD_SetCursor(uint8_t row, uint8_t col);
void LCD_Print(char *str);
void LCD_PrintChar(char ch);
void LCD_Backlight(uint8_t on);


#endif /* INC_LCD_H_ */
