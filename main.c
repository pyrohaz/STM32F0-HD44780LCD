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
#define BOUNCING_TEXT

//Timekeeping variable
volatile uint32_t MSec = 0;

//Millisecond counter interrupt using
//the internal SysTick timer
void SysTick_Handler(void){
	MSec++;
}

//Standard delay function! Executes the
//nop instruction until T milliseconds have
//passed.
void Delay(uint32_t T){
	volatile uint32_t MSS = MSec;
	while((MSec-MSS)<T) asm volatile("nop");
}

//Our standard 8 bit value write. The HD44780
//works in both 8bit and 4bit data modes. 4bit
//mode obviously requiring fewer pins from the
//microcontroller at the expense of slightly
//slower speeds and increased complexity. The
//most significant bits are sent first (8,7,6,5)
//with the least significant bits following second
//(4,3,2,1). Data is latched into the LCD using
//the enable pin H_EN. If RD is equal to 1, the
//value will be written to the DDRAM (for characters).
//If RD is equal to 0, the data will be written to the
//instruction registers.
void H_W8b(uint8_t Data, uint8_t RD){
	GPIO_ResetBits(HD44780_GPIO, H_D1|H_D2|H_D3|H_D4);
	GPIO_WriteBit(HD44780_GPIO, H_RS, RD);
	GPIO_SetBits(HD44780_GPIO, H_EN);

	GPIO_WriteBit(HD44780_GPIO, H_D4, Data&(1<<7));
	GPIO_WriteBit(HD44780_GPIO, H_D3, Data&(1<<6));
	GPIO_WriteBit(HD44780_GPIO, H_D2, Data&(1<<5));
	GPIO_WriteBit(HD44780_GPIO, H_D1, Data&(1<<4));

	GPIO_ResetBits(HD44780_GPIO, H_EN);

	GPIO_SetBits(HD44780_GPIO, H_EN);

	GPIO_WriteBit(HD44780_GPIO, H_D4, Data&(1<<3));
	GPIO_WriteBit(HD44780_GPIO, H_D3, Data&(1<<2));
	GPIO_WriteBit(HD44780_GPIO, H_D2, Data&(1<<1));
	GPIO_WriteBit(HD44780_GPIO, H_D1, Data&(1<<0));

	GPIO_ResetBits(HD44780_GPIO, H_EN);

	Delay(1);
}

//Really simple function to find the length of
//a string. Saves importing the string.h library
//for one function! Returns the string length
//excluding the null terminator (hence the -1)
uint8_t Strlen(const char* S){
	int8_t Cnt = 0, CChar = 1;

	//Run through the string until the character
	//is equal to the null terminator (0).
	while(CChar){
		CChar = S[Cnt];
		Cnt++;
	}

	//Return the length of the string, excluding
	//the null terminator.
	return Cnt-1;
}

//A pretty simple function to print strings to
//the display. The strings are sent to the function
//as a pointer, ensuring the last character is a
//null terminator (0). The X and Y positions are
//also sent and tests are done to ensure these are
//in range of the screen.
int8_t PStr(const char* S, uint8_t X, uint8_t Y){
	uint8_t Cnt, StrLen;

	//Get the length of the string
	StrLen = Strlen(S);

	//If the string is too long, return -3
	if(StrLen>15) return -3;

	//If the total string length will be out of
	//the screen, return -1
	if(X>(16-StrLen)) return -1;

	//If the row is anything other than 1 or 2,
	//return -2
	if(Y!=1 && Y!=2) return -2;

	//If all the above checks are ok, set the DDRam
	//address dependent on X position and row
	if(Y == 1) H_W8b(H_SetDDRamAdd|X, 0);
	else H_W8b(H_SetDDRamAdd|(0x40+X), 0);

	//Print the string to DDRam character by
	//character! The DDRam address automatically
	//increments, as defined by the initial register
	//values.
	for(Cnt = 0; Cnt<StrLen; Cnt++){
		H_W8b(S[Cnt], 1);
	}

	//If all is successful, return 0!
	return 0;
}

//Print a single character at the position X, Y
//where X is the X position of the row (0 being
//the first position and 15 being the last) with
//Y being the row, either row 1 or row 2.
int8_t PChar(char C, uint8_t X, uint8_t Y){

	//If the position of the character will be
	//off the screen, return respective error.
	if(X>14) return -1;
	if(Y!=1 && Y!=2) return -2;

	//If the row is row 1, the address range
	//is 0 to 15. If the row is row 2, the
	//address range is 64 to 79 with 64 being
	//the co-ordinates (0, 1) and 79 being
	//(15, 1).
	if(Y == 1) H_W8b(H_SetDDRamAdd|X, 0);
	else H_W8b(H_SetDDRamAdd|(0x40+X), 0);

	//Write the current character to the DDRam
	//as opposed to an instruction register
	H_W8b(C, 1);

	return 1;
}

//A simple function to clear the whole display.
//You could essentially print a string of spaces
//but this function does it for you - and much
//faster!
void ClrDisp(void){
	H_W8b(H_ClearDisp, 0);
	Delay(1);
}

//GPIO type definition
GPIO_InitTypeDef G;

//Lé main loop!
int main(void)
{
	//Send the clock to the GPIOA peripheral for the
	//HD44780 pins, change this dependent on which
	//GPIO you use.
	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_GPIOA, ENABLE);

	//Setup the Systick timer for 1ms interrupts.
	SysTick_Config(SystemCoreClock/1000);

	//Initialize all HD44780 pins as outputs.
	//The interface is pretty slow so lets cut
	//out the potential for all that EMI by setting
	//the output speed to the slowest (2MHz - also
	//defined in the datasheet as the fastest rate
	//for the HD44780 data port).
	G.GPIO_Pin = H_RS|H_EN|H_D1|H_D2|H_D3|H_D4;
	G.GPIO_Mode = GPIO_Mode_OUT;
	G.GPIO_OType = GPIO_OType_PP;
	G.GPIO_PuPd = GPIO_PuPd_NOPULL;
	G.GPIO_Speed = GPIO_Speed_Level_1;
	GPIO_Init(HD44780_GPIO, &G);

	//Initialize the outputs.
	GPIO_ResetBits(HD44780_GPIO, H_RS|H_D1|H_D2|H_D3|H_D4);
	GPIO_SetBits(HD44780_GPIO, H_EN);

	//Delay required for power supply stability.
	Delay(50);

	//Initialize the screen in 4bit data interface mode
	H_W8b(H_SetFunction|H_DataLength4b, 0);
	Delay(5);

	//Set the amount of display lines to 2 and the
	//character font size to 5x8 (5 pixels by 8 pixels
	//per character).
	H_W8b(H_SetFunction|H_DataLength4b|H_DispLines2|H_CharFont5x8, 0);
	Delay(1);

	//Set the DDRam address to automatically increment.
	//Disable the display shift.
	H_W8b(H_EntryModeSet|H_Increment|H_DispShiftDis, 0);
	Delay(1);

	//Clear the display before turning it on.
	ClrDisp();

	//If bouncing text is enabled, disable the blinking
	//cursor. Otherwise, enable the blinking cursor.
#ifdef BOUNCING_TEXT
	H_W8b(H_DispCtrl|H_DispOn|H_CursorOff|H_CursrPosNBlnk, 0);
#else
	H_W8b(H_DispCtrl|H_DispOn|H_CursorOff|H_CursrPosBlnk, 0);
#endif

	Delay(1);

	//Print the string to the screen at 0,1!
	PStr("Hello World!", 0, 1);

	//Initial variables for bouncing text
	uint8_t X = 0, Y = 1;

	//An XDir of 1 means the text is moving forward!
	int8_t XDir = 1;

	while(1)
	{
//If bouncing text is defined, run the bouncing text
//otherwise, do nothing!
#ifdef BOUNCING_TEXT
		//Increment the X variable by "XDir" amount
		X+=XDir;

		//If X has reached the screen boundaries,
		//reverse the direction that the text moves
		//by changing the sign infront of XDir.
		if(X>3 || X<1){
			XDir = -XDir;
		}

		//Clear the display
		ClrDisp();

		//Print the hello world string
		PStr("Hello World!", X, Y);

		//Delay for 500ms
		Delay(500);
#endif
	}
}
