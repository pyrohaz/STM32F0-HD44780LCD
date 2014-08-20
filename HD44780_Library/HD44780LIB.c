#include <HD44780LIB.h>

/*
 * HD44780LIB.c
 * Author: Harris Shallcross
 * Year: ~18/8/2014
 *
 *A pretty simple library used to interface a HD44780 LCD
 *with an STM32F0 Discovery board using standard GPIO pins.
 *The library offers functionality of providing a negative
 *voltage required for good contrasts at lower supply
 *voltages. This is generated using a charge pump and a
 *simple external circuit. The library also offers 4bits
 *of backlight brightness control through simple PWM.
 *The library is dependent on a SysTick interrupt and a
 *1ms Delay function provided in the example code.
 *The library also offers simple functions to print strings,
 *characters and numbers, along with clearing the display
 *and a few simple math functions meaning that minimal
 *external libraries are required.
 *
 *Function descriptions can be found on my blog at:
 *www.hsel.co.uk
 *
 *The code is released under the CC-BY license.
 *
 *This code is provided AS IS and no warranty is included!
 */

//LED brightness variable, allows 16 different
//steps of backlight brightness (1 to 16)!
volatile uint8_t LEDBrightness = 16;

//The handler for the LED PWM output. Place this
//inside of the Systick interrupt!
void H_LEDPWM(void){
	static uint8_t LEDCnt = 0;

	//Increment the LED Counter
	LEDCnt++;

	//If the counter is less than the brightness
	//variable, output a high, otherwise output
	//a low. The poor mans PWM! The PWM will
	//operate at a frequency of 1000/16 = 62.5Hz
	if(LEDCnt<LEDBrightness){
		GPIO_SetBits(HD44780_GPIO, H_LEDCtrl);
	}
	else{
		GPIO_ResetBits(HD44780_GPIO, H_LEDCtrl);
	}

	//Rollover once the LEDCnt gets to 16.
	LEDCnt &= 15;
}

//Toggle the Charge pump pin, creating the negative
//voltage required for the screen contrast.
void H_ChargePump(void){
	static uint8_t CPState = 1;

	//Write the current charge pump pin state
	GPIO_WriteBit(HD44780_GPIO, H_ChgPmp, CPState);

	//Invert the value of the current state
	CPState^=1;
}

//GPIO type definition for HW initialization.
GPIO_InitTypeDef G;

//Hardware initialization function, change parameters
//here if different pins are used!
void H_HWInit(void){
	//Send the clock to the GPIOA peripheral for the
	//HD44780 pins, change this dependent on which
	//GPIO you use.
	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_GPIOA, ENABLE);

	//Initialize all HD44780 pins as outputs.
	//The interface is pretty slow so lets cut
	//out the potential for all that EMI by setting
	//the output speed to the slowest (2MHz - also
	//defined in the datasheet as the fastest rate
	//for the HD44780 data port).
	G.GPIO_Pin = H_RS|H_EN|H_D1|H_D2|H_D3|H_D4|H_LEDCtrl|H_ChgPmp;
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

	//Enable the screen!
	H_W8b(H_DispCtrl|H_DispOn|H_CursorOff|H_CursrPosNBlnk, 0);
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

//A (not so) fast power function for all integer
//powers larger than 0. If a non positive power
//is given as the Pow parameter, 0 will be
//returned (impossible I know!)
int32_t FPow(int32_t Num, uint32_t Pow){
	int32_t NumO = Num;
	uint32_t Cnt;
	if(Pow>1){
		for(Cnt = 0; Cnt<Pow-1; Cnt++){
			Num*=NumO;
		}
		return Num;
	}

	if(Pow==1) return Num;
	if(Pow==0) return 1;
	else return 0;
}

//A simple Abs function. It saves MCU space
//as I don't need to import the whole Math
//library.
int32_t Abs(int32_t Num){
	if(Num>0) return Num;
	else return -Num;
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
	if(StrLen>(H_XSize)) return -3;

	//If the total string length will be out of
	//the screen, return -1
	if(X>(H_XSize-StrLen)) return -1;

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

	//If all is successful, return current X position!
	return X+Len;
}

//Print a single character at the position X, Y
//where X is the X position of the row (0 being
//the first position and 15 being the last) with
//Y being the row, either row 1 or row 2.
int8_t PChar(char C, uint8_t X, uint8_t Y){

	//If the position of the character will be
	//off the screen, return respective error.
	if(X>(H_XSize-1)) return -1;
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

	//If all is successful, current X position will be returned!
	return X+1;
}

//A simple function that compares a number to
//successive powers of 10 to find the base 10
//length of the number!
uint8_t CheckNumLength(int32_t Num){
	uint8_t Len = 0, Cnt;

	//Find length of number by continuously
	//comparing to ascending powers of 10.
	//When the number is less than the power of
	//10, the length of the number is found and
	//the loop is complete. 32bits supports
	//numbers up to 10dp so that is my max loop
	//size! If a 64bit system is used, this can be
	//increased though the FPow function will need
	//to be changed.
	for(Cnt = 0; Cnt<10; Cnt++){
		if(Num>=FPow(10, Cnt)){
			Len = Cnt;
		}
		else{
			Len = Cnt;
			break;
		}
	}

	return Len;
}

//A cool function to print numbers to the screen.
//I'm pretty proud of this one, as simple as it is!
//The number is split into its individual digits
//e.g. 123 becomes '1', '2', '3' and these are
//printed to the screen from most significant digit
//to least significant digit. Checks are done to add
//number padding or the negative sign (if numbers are
//negative).
int8_t PNum(int32_t Num, uint8_t X, uint8_t Y, uint8_t Pad){
	int8_t Cnt = 0, Len = 0, NegNum = 0;

	//Check to see if number is negative, if so
	//Abs the number and set the negative number
	//flag. If positive, NegNum will already be
	//0 as declared above.
	if(Num<0){
		Num = -Num;
		NegNum = 1;
	}

	Len = CheckNumLength(Num);

	//If the length of the number will print
	//off the screen, return respective error.
	if(X>((H_XSize-1)-Len)) return -1;
	if(Y!=1 && Y!=2) return -2;

	//If the row is row 1, the address range
	//is 0 to 15. If the row is row 2, the
	//address range is 64 to 79 with 64 being
	//the co-ordinates (0, 1) and 79 being
	//(15, 1).
	if(Y == 1) H_W8b(H_SetDDRamAdd|X, 0);
	else H_W8b(H_SetDDRamAdd|(0x40+X), 0);

	//If the negative number flag is not equal to
	//zero then print the - sign before the padding
	//and numbers
	if(NegNum){
		H_W8b('-', 1);
	}

	//Print number padding before the actual number
	//values. Useful for time! e.g. 08, 09, 10, 11
	//as it looks much more professional having the
	//0 as a place holder.
	for(Cnt = 0; Cnt<Pad; Cnt++){
		H_W8b('0', 1);
	}

	//Print the actual digits to the number!
	for(Cnt = Len-1; Cnt>=0; Cnt--){
		//Write the current number character to
		//the DDRam as opposed to an instruction
		//register. The magic happens in how the
		//number character is calculated utilizing
		//the truncating integer division and a mod
		//function. The division could be changed
		//for a while loop subtracting 10^Cnt until
		//the number is < that value.
		H_W8b('0'+ ((Num/FPow(10, Cnt))%10), 1);
	}

	//As per, return current X if all is good!
	return X+Len;
}

//A somewhat simple fucntion to print floating point
//numbers! Its undefined for times where the precision
//exceeds the decimal section of the number e.g.
//if you try and display 1.01 with a precision of 3.
//You should always try and display the decimal number
//to the precision you want or less!
int8_t PNumF(float Num, uint8_t X, uint8_t Y, uint8_t Prec){
	int32_t INum, DNum;
	uint32_t PrecPow = FPow(10, Prec);
	uint8_t Len = 0, Cnt, OriginX, NegNum = 0;

	//If the specified row doesn't exist, return value!
	if(Y!=1 && Y!=2) return -2;

	//If number is less than zero, print '-' sign
	//and invert the number.
	if(Num<0.0f){
		NegNum = 1;
		Num = -Num;
	}

	INum = Num;

	//If the integer section of the number is 0
	//Print the leading zero. Without this, only
	//a decimal point will be printed!
	if(INum==0){
		if((Prec+2+NegNum)>(H_XSize-X)){
			return - 1;
		}

		//If the negative number flag is set, print
		//the '-' sign
		if(NegNum){
			PChar('-', X, Y);

			//As a new character is printed, increment
			//the X position.
			X++;
		}

		PChar('0', X, Y);

		//Increment the position of X as a new
		//character has been printed.
		X++;
	}
	else{
		//If the integer part of the number isn't
		//equal to 0, find the length of the number
		//and print it! Increment the X position by
		//the length of the number.
		Len = CheckNumLength(INum);

		if((Len+Prec+2+NegNum)>(H_XSize-X)){
			return - 1;
		}

		//If the negative number flag is set, print
		//the '-' sign
		if(NegNum){
			PChar('-', X, Y);

			//As a new character is printed, increment
			//the X position.
			X++;
		}

		PNum(INum, X, Y, 0);
		X+=Len;
	}

	//If the precision specified is more than 0
	if(Prec>0){
		//Print the decimal point after the integer
		//part of the number.
		PChar('.', X, Y);

		//Increment the position of the X counter
		X++;

		//Multiply the floating point number by
		//10^precision - this shifts the numbers
		//below the decimal point, over the decimal
		//point
		Num*=(float)PrecPow;

		//Convert the floating point number into an
		//integer, this truncates the decimal values
		DNum = Num;

		//Subtract the initial integer part of the
		//number which won't contain the newly shifted
		//decimal values.
		DNum-=INum*PrecPow;

		//Check the length of the new decimal part of
		//the number
		Len = CheckNumLength(DNum);

		//If this number length is less than the specified
		//precision, pad with leading zeros and increment
		//the current X position.
		if(Len<Prec){
			for(Cnt = Len; Cnt<Prec; Cnt++){
				PChar('0', X, Y);
				X++;
			}
		}

		PNum(DNum, X, Y, 0);
		X+=Len;
	}

	//If all printed well, return current X position to indicate success!
	return X+Len;
}

//A simple function to clear the whole display.
//You could essentially print a string of spaces
//but this function does it for you - and much
//faster!
void ClrDisp(void){
	H_W8b(H_ClearDisp, 0);
	Delay(1);
}
