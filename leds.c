#include <avr/io.h>
#define F_CPU 16000000UL
#include <util/delay.h>

//Code to run a MAX7221 bitbanged from an AVR port.
//Kept for reference, along with Makefile and a few other programs.
//Good example of a bitbanged SPI implementation.
#define CS PD3
#define DIN PD4
#define CLK PD5


//Should probably time this properly to see how many ns it takes. 
void shiftShort(short data){
	PORTD &= ~(1<<CS); // Bring CS low to signal to the chip we want to talk.
	for (int i = 15; i >= 0; --i) // Go through the short MSB first.
	{
		if(data&(1<<i)){ // If that bit is 1..,
			PORTD |=(1<<DIN);//set data line high
		}else{ 
			PORTD &= ~(1<<DIN);//Set data line low
		}
		PORTD |=(1<<CLK); // Clock that data out.
		if(i==0){ //If we're on the last bit, the CS line must go high before the clock goes low, or we lose all the data.
				PORTD |= (1<<CS); // We're done with the chip
		}

		PORTD &=~(1<<CLK); //Get ready for the next bit

	}
}


//Create the right magic short to put a number in a position, optionally with a decimal point beside it.
void setDigit(uint8_t pos, uint8_t num, uint8_t dec){
	short dat = 0;
	dat = (pos<<8);
	if(dec){
		dat |= (1<<7);
	}
	dat |= num;
//	return dat;
	shiftShort(dat);
}
void main(){

	DDRD |= (1<<PD5)|(1<<PD4)|(1<<PD3);
	PORTD |= (1<<CS);

	//Magic sauce to get the display to turn on.
	shiftShort(0x0A01); // <-- change 01 to anything from 01 to 0F for different brightness levels
	shiftShort(0x09FF); // Enable BCD decoding.
	shiftShort(0x0B03);	// <-- change 03 if you've got more or less than 4 digits (zero indexed)
	shiftShort(0x0C01); // Turn the display on from sleep mode!

	setDigit(3,1,0); // Set a digit to test it out.

	while(1){
	}
}