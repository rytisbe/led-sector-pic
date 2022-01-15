// MCU			- PIC16F1829
// LED Driver	- MAX6969
// Name			- Rytis Beinarys
// Description	- LED Animations

#include "config.h"
#include <stdio.h>
#include <string.h>

#define LE		RC7
#define DATA	RB5
#define CLOCK	RB7
#define ON		1
#define OFF		0

#define _XTAL_FREQ	16000000	// Internal Oscillator Frequency
#define	BRIGHTNESS	CCPR1L		// 255 - OFF	|	0 - ON


const int LEDS		= 32;	// LEDs per layer
const int BYTES		= 4;	// bytes per layer
const int LAYERS	= 16;	// height of the tower
const int ISR_TIMER = 15531;

// ------------------    Global Variables    ----------------------

int g_LED_Counter	= 0;
int g_Counter		= 0;
unsigned char g_temp[1][4];


unsigned char LED_Buffer[16][4] ={0};

// ---------------    Function Declarations    --------------------
// -----------------    S ys Functions    --------------------

void Sys_Init(void);
void interrupt ISR(void);
unsigned int ADC(void);
void Update_Display(void);

// ----------------    Misc Functions    --------------------

void delay(int);
void Clear_Buffer(void);
void Invert_Display(void);

// ---------------    Display Functions    ------------------

void Switch_Pixel(char, char, char);
char Get_Pixel(char, char);
void Switch_Row(char, char);
void Switch_Col(char, char);
void Shift_Right(void);
void Shift_Down(char);
unsigned int countSetBits(unsigned long n);

// ----------------    LED Animations    --------------------

void LED_Paparazzi(void);
void LED_VSweep(void);
void LED_H_Sweep(void);
void LED_Diagonal(void);
void LED_Checkerboard(void);
void LED_Swirl(void);
void LED_Arrow_LShift(void);
void LED_Sine_Wave(void);
void LED_Text(void);
void LED_New(void);

// ---------------------    Super Loop    -------------------------

void main(){
	
	// ----------------
	int seed = 23;
	srand(seed);
	// ----------------
	
	Sys_Init();
	
	
	
	BRIGHTNESS = 220;
	STKPTR++;
	while(1){
		switch(g_LED_Counter) {
			//case 0:			LED_New();			break;
			case 8:			LED_Text();			break;
			case 1:			LED_Paparazzi();	break;
			case 2:			LED_H_Sweep();		break;
			case 3:			LED_Diagonal();		break;
			case 4:			LED_VSweep();		break;
			case 5:			LED_Checkerboard();	break;
			case 6:			LED_Swirl();		break;
			case 7:			LED_Arrow_LShift();	break;
			case 0:			LED_Sine_Wave();	break;
			
			default:		g_LED_Counter = 0;	 
		}
	}  
}

// ----------------    Function Definitions    --------------------

// ------------    System Initialisation    ------------
void Sys_Init(){
	
	// System Clock
	OSCCON		=	0b01111010;		// Internal Oscillator			[ 16MHz ]
	
	// Port Directions 
	TRISA		=	0b00000000;		// PORTA						[ Output ]
	TRISB		=	0b00000000;		// PORTB						[ Output ]
	TRISC		=	0b01000000;		// PORTC						[ Output/Input ]
	
	// Analogue / Digital Module 
	ANSELC		=	0b01000000;		// ANSC7	|	LDR or POT		[ Analogue ]
	ADCON0		=	0b00100001;		// A/D Control Register 0		[ AN8 Enabled ]
	ADCON1		=	0b10100000;		// A/D Clock (FOSC/32)			[Right Justified]
	
	// EUSART / Baud Rate 
	SPBRGH		=	0b00000000;	
	SPBRGL		=	0b00000100;		// SPBRGH:SPBRGL = 3			[ 1000000 b/s ]
	BAUDCON		=	0b00010000;		// Clock Polarity				[ Falling Edge ] 
	RCSTA		=	0b10000000;		// RX Mode Disabled
	TXSTA		=	0b10110000;		// Synchronous Master Serial	[ TX Mode ]
	
    // Timer0 | Interrupt
    INTCON      =   0b11100000;     // Interrupt Configuration Bits [ GIE | PEIE ]  
    OPTION_REG  =   0b11000011;     // FOSC/4   |   Prescaler Rate  [ 1:16 ] 		
	
	// PWM	|	Timer2
	CCP1CON		=	0b00111100;		// 2 LSBs of the PWM duty cycle
	CCPTMRS		=	0b00000000;		// PWM use Timer 2.
	PR2 = 255;	// 40 - 100kHz
	
	TMR2IF = 0;
	T2CON	= 0b00000100;			// TMR2 / Prescaler 4 / PWM Freq	[ 3.906 kHz ]

	GIE = 1;
	PEIE = 1;
}	

// --------------    Interrupt Routine    --------------
void interrupt ISR(void){
	if (TMR0IF && TMR0IE){			// (16MHz / (4 * 16 * 256))		[ 1.024 ms ]
		TMR0IF = 0;					// Clear Overflow Flag
		
		if (++g_Counter > ISR_TIMER){	// 19531 * 1.024 ms 			[ ~ 20 sec ]
			g_LED_Counter++;		// Change the animation at a fixed time interval
			g_Counter = 0;
			
			while(STKPTR > 1)		// Clear the stack down to the checkpoint in main()
			{
				TOSH = 0;
				TOSL = 0;
				STKPTR--;
			}
		}
	}
}

unsigned int ADC(void){
	delay(1);
	GO_nDONE = 1;
	while(GO_nDONE);
	return ((ADRESH << 8) + ADRESL) / 4;
}

void delay(int ms){
	for (; ms > 0; ms--)
		__delay_ms(1);
}

// ----------------    Update LEDs    -----------------
void Update_Display(void){
	
		while(!TRMT);			// Wait until transmission register is empty
		
		for (int y = 0; y < LAYERS; y++){
			for (int x = 0; x < BYTES; x++){
			TXREG = LED_Buffer[y][x];
			while (!TRMT);
			}
		}
			
		
		LE = 1;					// Data is loaded to the Output latch
		NOP();	NOP();			// (2 x 250 ns) instructions	[tLRF - 310 ns]
		LE = 0;					// Data is latched into the Output latch
}

void Clear_Buffer(void){
	for (int y = 0; y < LAYERS; y++){
		for (int x = 0; x < BYTES; x++){
			LED_Buffer[y][x] = 0b00000000;
		}
	}
}

void Shift_Down(char d){
		
		int y,x;

		for (int y = 0; y < BYTES; y++)
			g_temp[0][y] = LED_Buffer[LAYERS-1][y];

		for(int y = LAYERS-1; y > 0; y--)
		{
			for (int x = 0; x < BYTES; x++)
			{
				LED_Buffer[y][x] = LED_Buffer[y-1][x];
			}
		}
		for (int y = 0; y < BYTES; y++)
		{
			LED_Buffer[0][y] = g_temp[0][y];
		}
		Update_Display();
		delay(d);
}



void Invert_Display(void){
	for (int y = 0; y < LAYERS; y++){
		for (int x = 0; x < BYTES; x++){
			LED_Buffer[y][x] ^= (0b11111111);
		}
	}
	Update_Display();
}

void Switch_Pixel(char y, char x, char state) {
		
		if (state)
			LED_Buffer[y][x/8] |= (1 << x % 8);
		
		else if (!state) 
			LED_Buffer[y][x/8] &= ~(1 << x % 8);
		else
			;	// correct?!
		Update_Display(); 
}

char Get_Pixel(char y, char x) {
		
	if (((LED_Buffer[y][x/8]) &= 1 << (x % 8)) == 1)
		return 1;
	else
		return 0;
}


void Switch_Row(char l, char state){

	int x;
	
	if (state == ON){
		for (x = 0; x < 4; x++)
			LED_Buffer[l][x] = 0b11111111;
	}
	else {
		for (x = 0; x < 4; x++)
			LED_Buffer[l][x] = 0b00000000;
	}
	Update_Display();
}

void Switch_Col(char c, char state){
	
	int y;
	
	if (state == ON){
		for (y = 0; y < LAYERS; y++)
			LED_Buffer[y][c/8] |= (1 << c % 8);
	}
	else{
		for (y = 0; y < LAYERS; y++)
			LED_Buffer[y][c/8] &= ~(1 << c % 8);
	}	
	Update_Display();
}

unsigned int countSetBits(unsigned long n)
{
    unsigned int count = 0;
    while (n)
    {
      n &= (n-1) ;
      count++;
    }
    return count;
}

void Shift_Right(void) 
{
	for (int row = 0; row < LAYERS; row++) {
		unsigned char carry = LED_Buffer[row][BYTES - 1] >> 7 ;
		for( int i = 0; i < BYTES; i++ )
		{   
			unsigned char next_carry = LED_Buffer[row][i] >> 7 ;
			LED_Buffer[row][i] = (LED_Buffer[row][i] << 1) | carry ;
			carry = next_carry ;
		}
	}
}

// ---------------------    LED Animations    ---------------------


void LED_Paparazzi(void){
	
	int i,j;
	BRIGHTNESS = 50;

		Clear_Buffer();
		i = (rand() % LAYERS);
		j = (rand() % (LEDS));
		Switch_Pixel(i,j, ON);
		delay(50);
		Switch_Pixel(i,j, OFF);
		delay(50);
}

void LED_VSweep(void)
{
	BRIGHTNESS = 240;
	
	char l		= 0;
	char state	= 1;
	char check	= OFF;
	
	while (l < LAYERS && check != 2)
	{
		Switch_Row(l, state);	delay(15);	l++;
		
		if (l == LAYERS){
			state = ~state;		l = 0;			check++;
		}
	} 
	
	l		= LAYERS;
	state	= 1;
	
	while (l >= 0 && check != 4)
	{
		l--;	Switch_Row(l, state);	delay(80);
			
		if (l == 0){
			state = ~state;		l = LAYERS;		check++;
		}
	}
}

void LED_H_Sweep(void)
{	
	BRIGHTNESS = 240;
	Clear_Buffer();
	
	int i, y, x;	
	
	for (i = 0; i < 40; i++)
	{	
		y = rand() % LAYERS;				// random y position
		x = (rand() % (28 - 4 + 1)) + 4;	// random x position
		
		Switch_Pixel(y, x, ON);
		delay(50);
	}
			
	delay(500);
	
	for (i = 0; i < (LEDS/2); i++)
	{
		Switch_Col(i, ON);
		Switch_Col(((LEDS - 1) - i), ON);
		
		delay(40);
		
		Switch_Col(i, OFF);
		Switch_Col(((LEDS - 1) - i), OFF);
	}

	for (i = (LEDS/2); i >= 0; i--)
	{
		Switch_Col(i, ON);
		Switch_Col(((LEDS) - i), ON);
		
		delay(40);
		
		Switch_Col(i, OFF);
		Switch_Col(((LEDS) - i), OFF);
	}
}

void LED_Checkerboard(void){
	
	Clear_Buffer();
	BRIGHTNESS = 240;
	
	delay(200);
	
	for (int y = 0; y < (LAYERS/2+1); y++)
	{
		for (int x = 0; x < LEDS/2; x++)
		{
			Switch_Pixel(y*2, (x*2), ON);
			Switch_Pixel((15 - (y*2)), (x*2+1), ON);
			
		}
		delay(100);
	}
	delay(1000);
	
	while (BRIGHTNESS++ < 255)
	{
		Invert_Display();
		delay(150);
		if (BRIGHTNESS == 255){
			Clear_Buffer();
			Update_Display();
		}
	}
	delay(200);
	
}

void LED_Diagonal(void){
	Clear_Buffer();
	BRIGHTNESS = 240;
	static char dir = 0;
	
	Switch_Row(LAYERS-1, ON);
	Switch_Row(0, ON);
	
	if (dir)
	{
		Switch_Row(LAYERS-1, ON);
		for (int i = 0; i < (LAYERS); i++)
		{	
		delay(100);
		

		for (int o = 0; o < 6; o++)
			Switch_Pixel((i), (o*5)+i, ON);
	
		}
		
		for (int i = 0; i < (LAYERS-2); i++)
		{
			Switch_Row(i, ON);
			delay(20);
			Switch_Row(i, OFF);
		}
		for (int i = (LAYERS-2); i > 0; i--)
		{
			Switch_Row(i, ON);
			delay(20);
			Switch_Row(i, OFF);
		}
	}
	else
	{
		for (int i = 0; i < LAYERS; i++)
		{	
		delay(100);

		for (int o = 0; o < 6; o++)
			Switch_Pixel(((LAYERS-1)-i), (o*5)+i, ON);
	
		}
		for (int i = (LAYERS-2); i > 0; i--)
		{
			Switch_Row(i, OFF);
			delay(100);
			
		}
	}	
	dir = ~dir;
}

void LED_Swirl(void)
{
	BRIGHTNESS = 240;
	
	static const unsigned char buffer[16][4] =
{
	0x01, 0x00, 0x01, 0x00, 0x02, 0x80, 0x02, 0x80, 0x04, 0x40, 0x04, 0x40, 0x08, 0x20, 0x08, 0x20,
	0x10, 0x10, 0x10, 0x10, 0x20, 0x08, 0x20, 0x08, 0x40, 0x04, 0x40, 0x04, 0x80, 0x02, 0x80, 0x02,
	0x00, 0x01, 0x00, 0x01, 0x80, 0x02, 0x80, 0x02, 0x40, 0x04, 0x40, 0x04, 0x20, 0x08, 0x20, 0x08,
	0x10, 0x10, 0x10, 0x10, 0x08, 0x20, 0x08, 0x20, 0x04, 0x40, 0x04, 0x40, 0x02, 0x80, 0x02, 0x80
};
		
	memcpy(LED_Buffer, buffer, sizeof(buffer));
	
	while(1){
		Shift_Down(70);
	}
}
void LED_Text(void)
{
	BRIGHTNESS = 240;
	
	static const unsigned char buffer[16][4] =
	{
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x3c, 0x18, 0xf6, 0xcd, 0x7e, 0x18, 0xf6, 0xcd,
	0x66, 0x18, 0x86, 0xcd, 0x66, 0x18, 0x86, 0xcd, 0x66, 0x18, 0x86, 0xcd, 0x66, 0x18, 0xf6, 0xfd,
	0x66, 0x18, 0xf6, 0xfd, 0x66, 0x18, 0x86, 0xcd, 0x66, 0x18, 0x86, 0xcd, 0x66, 0x18, 0x86, 0xcd,
	0x7e, 0xdf, 0xf7, 0xcd, 0x3c, 0xdf, 0xf7, 0xcd, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};
		
	memcpy(LED_Buffer, buffer, sizeof(buffer));
	
	while(1) {
		Shift_Right();
		Update_Display();
		delay(60);
	}
}

void LED_Arrow_LShift(void){
	
	BRIGHTNESS = 240;
	
    static const unsigned char buffer[16][4] = 
    {
	0x81, 0x81, 0x81, 0x01, 0x03, 0x03, 0x03, 0x03, 0x06, 0x06, 0x06, 0x06, 0x0c, 0x0c, 0x0c, 0x0c,
	0x18, 0x18, 0x18, 0x18, 0x30, 0x30, 0x30, 0x30, 0x60, 0x60, 0x60, 0x60, 0xc0, 0xc0, 0xc0, 0xc0,
	0x81, 0x81, 0x81, 0x81, 0xc0, 0xc0, 0xc0, 0xc0, 0x60, 0x60, 0x60, 0x60, 0x30, 0x30, 0x30, 0x30,
	0x18, 0x18, 0x18, 0x18, 0x0c, 0x0c, 0x0c, 0x0c, 0x06, 0x06, 0x06, 0x06, 0x03, 0x03, 0x03, 0x03
};

    memcpy(LED_Buffer, buffer, sizeof(buffer));
	
	while(1){
		for (int i = 80; i > 10; i--){
			Shift_Right();
			Update_Display();
			delay(i);
		}
		for (int i = 50; i > 0; i--){
			Shift_Right();
			Update_Display();
			delay(10);	
		}
		for (int i = 9; i < 80; i++){
			Shift_Right();
			Update_Display();
			delay(i);
		}
	}
}


void LED_Sine_Wave(void){
	static const unsigned char buffer[16][4] = 
{
	0x00, 0x00, 0x1e, 0x00, 0x00, 0x00, 0x0f, 0x00, 0x00, 0x80, 0x07, 0x00, 0x00, 0xc0, 0x03, 0x00,
	0x00, 0xe0, 0x01, 0x00, 0x00, 0xf0, 0x00, 0x00, 0x00, 0x78, 0x00, 0x00, 0x00, 0x3c, 0x00, 0x00,
	0x00, 0x78, 0x00, 0x00, 0x00, 0xf0, 0x00, 0x00, 0x00, 0xe0, 0x01, 0x00, 0x00, 0xc0, 0x03, 0x00,
	0x00, 0x80, 0x07, 0x00, 0x00, 0x00, 0x0f, 0x00, 0x00, 0x00, 0x1e, 0x00, 0x00, 0x00, 0x3c, 0x00
};
	
	memcpy(LED_Buffer, buffer, sizeof(buffer));
	Update_Display();
	
	while(1)
	{
		Shift_Down(80);
		Update_Display();
	}
}

void LED_New(void){
	Clear_Buffer();
	
	static const unsigned char buffer0[16][4] = {
0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x01, 0x00, 0x00, 0xc0, 0x03, 0x00, 0x00, 0x60, 0x06, 0x00, 0x00, 0x30, 0x0c, 0x00, 0x00, 0x18, 0x18, 0x00, 0x00, 0x0c, 0x30, 0x00, 0x00, 0x06, 0x60, 0x00, 0x00, 0x0c, 0x30, 0x00, 0x00, 0x18, 0x18, 0x00, 0x00, 0x30, 0x0c, 0x00, 0x00, 0x60, 0x06, 0x00, 0x00, 0xc0, 0x03, 0x00, 0x00, 0x80, 0x01, 0x00
};
	static const unsigned char buffer1[16][4] = {
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x01, 0x00, 0x00, 0xc0, 0x03, 0x00, 0x00, 0x60, 0x06, 0x00, 0x00, 0x30, 0x0c, 0x00, 0x00, 0x18, 0x18, 0x00, 0x00, 0x0c, 0x30, 0x00, 0x00, 0x18, 0x18, 0x00, 0x00, 0x30, 0x0c, 0x00, 0x00, 0x60, 0x06, 0x00, 0x00, 0xc0, 0x03, 0x00, 0x00, 0x80, 0x01, 0x00
};
	static const unsigned char buffer2[16][4] = {
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x01, 0x00, 0x00, 0xc0, 0x03, 0x00, 0x00, 0x60, 0x06, 0x00, 0x00, 0x30, 0x0c, 0x00, 0x00, 0x18, 0x18, 0x00, 0x00, 0x30, 0x0c, 0x00, 0x00, 0x60, 0x06, 0x00, 0x00, 0xc0, 0x03, 0x00, 0x00, 0x80, 0x01, 0x00
};
	static const unsigned char buffer3[16][4] = {
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x01, 0x00, 0x00, 0xc0, 0x03, 0x00, 0x00, 0x60, 0x06, 0x00, 0x00, 0x30, 0x0c, 0x00, 0x00, 0x60, 0x06, 0x00, 0x00, 0xc0, 0x03, 0x00, 0x00, 0x80, 0x01, 0x00
};
	for (int i = 0; i < 16; i++) {
		
		memcpy(LED_Buffer+i, buffer0, sizeof(buffer0));
		Update_Display();	
		delay(500);
	}
	
}

