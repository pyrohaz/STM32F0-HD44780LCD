#include <HD44780LIB.h>

//Timekeeping variable
volatile uint32_t MSec = 0;

//Millisecond counter interrupt using
//the internal SysTick timer
void SysTick_Handler(void){
	static uint8_t MState = 0;

	//Execute the LED PWM handler
	H_LEDPWM();

	//Execute charge pump handler
	H_ChargePump();

	//As the Systick handler now runs at 0.5ms
	//interrupts, MSec needs to be incremented
	//every two Systick interrupts, this is done
	//by incrementing whenever MState = 1. By
	//XOR'ing MState with 1, it will toggle
	//0 and 1. Only when it is equal to 1 can MSec
	//be incremented.

	MState^=1;
	if(MState == 1) MSec++;
}

//Standard delay function! Executes the
//nop instruction until T milliseconds have
//passed.
void Delay(uint32_t T){
	volatile uint32_t MSS = MSec;
	while((MSec-MSS)<T) asm volatile("nop");
}

//Lé main loop!
int main(void)
{
	//Setup the Systick timer for 0.5ms interrupts.
	SysTick_Config(SystemCoreClock/2000);

	//Initialize the HD44780!
	H_HWInit();

	//If bouncing text is enabled, disable the blinking
	//cursor. Otherwise, enable the blinking cursor.
#ifdef BOUNCING_TEXT
	H_W8b(H_DispCtrl|H_DispOn|H_CursorOff|H_CursrPosNBlnk, 0);

	//Initial variables for bouncing text
	uint8_t X = 0, Y = 1;

	//An XDir of 1 means the text is moving forward!
	int8_t XDir = 1;
	char A = 0;
	int32_t N, P = 0;
#else
	H_W8b(H_DispCtrl|H_DispOn|H_CursorOff|H_CursrPosNBlnk, 0);
#endif

	Delay(1);

	//Print the string to the screen at 0,1!
	PStr("Hello world!", 0, 1);

	//Print the number 9 after the hello world message
	PNum(9, 14, 1, 0);
	//Print the number -1237 with 2 zeros of padding
	//on the second line, will print "-001237"
	PNum(-1237, 0, 2, 2);

	//Delay for 2 seconds until the Millisecond counter is
	//displayed.
	Delay(2000);

	//Clear the current display.
	ClrDisp();

	//Reset millisecond counter!
	MSec = 0;

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
		if(X>(15-12) || X<1){
			XDir = -XDir;
		}

		//Clear the display
		ClrDisp();

		//Print the hello world string
		PStr("Hello world!", X, Y);

		//Increment LED brightness for absolutely
		//no reason! :)
		LEDBrightness++;
		if(LEDBrightness>15)LEDBrightness = 1;
		//Delay for 500ms
		Delay(500);
#endif

		//Display millisecond counter!
		PNum(MSec, 0, 1, 0);
	}
}
