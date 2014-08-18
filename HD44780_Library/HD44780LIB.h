#ifndef HD44780LIB_H
#define HD44780LIB_H

#include <stm32f0xx_gpio.h>
#include <stm32f0xx_rcc.h>

//HD44780 GPIO pin defines, the HD44780
//requires no special function pins!
#define H_RS GPIO_Pin_0
#define H_EN GPIO_Pin_1
#define H_D1 GPIO_Pin_2
#define H_D2 GPIO_Pin_3
#define H_D3 GPIO_Pin_4
#define H_D4 GPIO_Pin_5

//Pin for controlling the LED backlight!
#define H_LEDCtrl GPIO_Pin_6

//Pin required for capacitive charge
//pump!
#define H_ChgPmp GPIO_Pin_7

#define HD44780_GPIO GPIOA

//HD44780 Register definitions
#define H_ClearDisp		0x01
#define H_RetHome		0x02
#define H_EntryModeSet	0x04
#define H_DispCtrl		0x08
#define H_CurDispShft	0x10

#define H_SetFunction	0x20
#define H_SetCGRAMAdd	0x40
#define H_SetDDRamAdd	0x80

//HD44780 Register options (Gah!)
#define H_Increment 	(1<<1)
#define H_Decrement		(0<<1)
#define H_AccDispShft	(1<<0)
#define H_DispShift		(1<<4)
#define H_CurrMove		(0<<4)
#define H_ShiftRight	(1<<3)
#define H_ShiftLeft		(0<<3)
#define H_DataLength8b	(1<<5)
#define H_DataLength4b	(0<<5)
#define H_DispLines2	(1<<3)
#define H_DispLines1	(0<<3)
#define H_CharFont5x10	(1<<2)
#define H_CharFont5x8	(0<<2)
#define H_DispOn		(1<<2)
#define H_DispOff		(0<<2)
#define H_CursorOn		(1<<1)
#define H_CursorOff		(0<<1)
#define H_CursrPosBlnk	(1<<0)
#define H_CursrPosNBlnk	(0<<0)
#define H_DispShiftDis	(0<<0)
#define H_DispShiftEn	(1<<0)

//Bouncing text define, uncomment this if
//you wish you see the bouncing text. I'm
//not using any display shifts for this.
//#define BOUNCING_TEXT

//Allow all source and header files using
//this library to access the LEDBrightness
//variable.
extern volatile uint8_t LEDBrightness;

//Allow the library to access the external Delay
//function.
extern void Delay(uint32_t);

//Hardware control functions
void H_HWInit(void);
void H_LEDPWM(void);
void H_ChargePump(void);

//Data control functions
void H_W8b(uint8_t, uint8_t);

//Character handling functions
int8_t PStr(const char*, uint8_t, uint8_t);
int8_t PChar(char, uint8_t, uint8_t);
int8_t PNum(int32_t, uint8_t, uint8_t, uint8_t);

//Display control functions
void ClrDisp(void);

#endif
