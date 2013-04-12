#include <p18f4550.h>
#include <delays.h>

#pragma config FOSC = INTOSC_HS
#pragma config PWRT = OFF
#pragma config BOR = OFF
#pragma config WDT = OFF
#pragma config LVP = OFF
#pragma config DEBUG = ON

//button defines
#define BTN1 0
#define BTN2 1
#define BTN3 2
#define BTN4 3

unsigned char usart_received_char; //received char
unsigned char usart_receive_flag = 0; // used to know when we received smth
unsigned char buttonStates = 0x00; //register for using onRelease for buttons

void init_usart(void);
void usart_writeC(unsigned char c);
void interruptLow();
int checkButtonState(int bitc);

void main(void)
{
	unsigned char buttonReleased = 0;
	//config for 8MHz
	OSCCONbits.IRCF2 = 1;
	OSCCONbits.IRCF1 = 1;
	OSCCONbits.IRCF0 = 1;
	TRISB=0x00; //portb output	
	TRISDbits.TRISD0 = 1;
	LATB=0x00;

	init_usart(); //init usart com
	LATBbits.LATB0 = 1;
    while (1)
    {
		//we poll for button states
		//we turn off the led when we send the command
        buttonReleased = checkButtonState(BTN1);
		if(buttonReleased == 1)
		{
			LATBbits.LATB0 = 0;
			usart_writeC('1');
		}

		buttonReleased = checkButtonState(BTN2);
		if(buttonReleased == 1)
		{
			LATBbits.LATB0 = 0;
			usart_writeC('2');
		}

		buttonReleased = checkButtonState(BTN3);
		if(buttonReleased == 1)
		{
			LATBbits.LATB0 = 0;
			usart_writeC('3');
		}
        
		buttonReleased = checkButtonState(BTN4);
		if(buttonReleased == 1)
		{
			LATBbits.LATB0 = 0;
			usart_writeC('4');
		}
    }
}

//onRelease function
int checkButtonState(int bitc)
{
	unsigned char mask = 0x01; //mask for accessing the bits for portD
	unsigned char maskStates = 0x01; //mask for accessing les states
	mask = mask << bitc;
	maskStates = mask << bitc;
	if((PORTD & mask) != 0) // if button pressed
	{
		Delay1KTCYx(20);//de bouncing effect; wait 10ms
		if((PORTD & mask) != 0) // if button still pressed
		{
			buttonStates = (buttonStates|maskStates); //mark button as pressed
			LATBbits.LATB0 = 1;	//light up led
		}
	}
	else if((buttonStates&maskStates) != 0) //if button was pressed
	{
		buttonStates = (buttonStates&(~maskStates)); // put state as 0
		return 1; //return that button is released
	}
	return 0;	
}

#pragma code interruptVectorLow = 0x18 //low priority interrupt address
void interruptVectorLow(void)
{
	_asm
	goto interruptLow
	_endasm
}

#pragma code
#pragma interrupt interruptLow

void interruptLow()//receive interrupt function
{
	if(PIR1bits.RCIF == 1)//interrupt generated by usart receive
	{
		if(RCSTA&0x06)//error check
		{
			RCSTAbits.CREN = 0; //overrun error = data not read in time; we clear cren to clear the error
			usart_received_char = RCREG; //read the error
			RCSTAbits.CREN = 1;
		}
		else
		{
			usart_received_char = RCREG; //read data
			usart_receive_flag = 1; //enable flag
		}
	}
}

void init_usart(void)
{
	usart_receive_flag = 0;
	TRISCbits.TRISC7 = 1; //usart rx pin is input
	TRISCbits.TRISC6 = 0; //usart tx pin is output
	SPBRGH = 0x00; //usart baud rate
	SPBRG = 51; //for 8MHz pic frequency
	BAUDCONbits.BRG16 = 0;
	RCSTAbits.CREN = 1; //enable receiver
	RCSTAbits.SPEN = 1; //enable serial port
	RCSTAbits.RX9 = 0; //receive 8 bits
	TXSTAbits.TXEN = 1; //transmit enable
	TXSTAbits.BRGH = 1; //high speed
	TXSTAbits.SYNC = 0; //async mode
	TXSTAbits.TX9 = 0; //send 8 bits

	//priorities
	RCONbits.IPEN = 1; //low priority interrupts enabled
	IPR1bits.RCIP = 0; //the usart receive interrupt is the lowest
	PIE1bits.RCIE = 1; //enable usart receive interrupt
	INTCONbits.GIEL = 1; //enable global interrupts
	INTCONbits.GIEH = 1;
	INTCONbits.GIE = 1;
	INTCONbits.PEIE = 1;
}

void usart_writeC(unsigned char c)
{
//while(!PIR1bits.TXIF);
//TXREG = c;
	TXSTAbits.TXEN = 0; //disable transmission
	TXREG = c; //copy data to be sent in reg
	TXSTAbits.TXEN = 1; //reenable transmission
	while(TXSTAbits.TRMT == 0) // wait till transmission ends
	{
		Nop();
	}
}