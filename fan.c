#include <avr/io.h>
#include <avr/interrupt.h>
#define F_CPU 8000000
#include <util/delay.h>

//Allows PWM control of a computer fan, and reading the RPM value.
//Good reference for use of timers for lots of thigns, interrupt-based serial, and interrupts.

int rot=0;
int count=0;

//Global buffer for string serial. Messy, should be passed in w/ a pointer, but this was fast.
char buf[40];

//Serial printing data
volatile int curpos=0;
volatile int buflen=1;
volatile int finished=1;

volatile unsigned long millis=0; // Keep track of millisecond data. Updated using a timer.

unsigned long lastrpmcheck=0; // Keep track of the last millis value when we checked RPM.

//Print a string using our variables and arrays.
void print(){
	buflen=strlen(buf)+1;
	curpos=0;
	finished=0;
}
int main(){

	DDRD |= (1<<PD4);	//PD4 used for PWM output
	DDRD &= ~(1<<PD2); //Make sure the RPM pin is an input (not required, but let's be verbose)
	PORTD |= (1<<PD2); //Set the pull-up resistor on the RPM pin 
	DDRD |= (1<<PD7); //Debug/status LED

	//PORTD |= (1<<PD4);
	TCCR1A |= ((1<<COM1B1)|(1<<COM1B0)|(1<<WGM10)); //Drive OC1B high on compare match. Fast PWM mode.
	TCCR1B |= (1<<WGM12)|(1<<CS11); //Clock div8 prescaler.

	TCCR0  |=(1<<WGM01)|(1<<CS01)|(1<<CS00); //CTC, no PWM, div64 clock prescaler.

	OCR0=125; // Millis compare value. 
	
	TIMSK|=(1<<OCIE0); //Enable millis interrupt.

	//OCR1A=0;
	OCR1B=200; //Start value for the fan.

	GICR |= (1<<INT0);  //Enable interrupt 0.
	MCUCR |= (1<<ISC01) | (1<<ISC00); // Rising edge of int0.

	SREG |= (1<<7); //These two lines enable interrupts.
	sei();


	UCSRC |= ((1<<URSEL)|(1<<UCSZ1)|(1<<UCSZ0)); //Serial magic sauce. Set # of data bits, and the URSEL register to get around odd quirk in this gen. of AVR archetecture.
	UBRRL=103; // 9600bps.
	UBRRH=0;
	UCSRB |= ((1<<RXEN) | (1<<TXEN) | (1<<UDRIE)); // Enable RX,TX, and transmit interrupt.
	
	int addor=1; // Rolling up and down.
	while(1){

		if(millis-lastrpmcheck >= 1000){ // More than one second since last check?
			int rpm=((float)count/(float)(millis-lastrpmcheck))*60000; // Count comes from the pin change interrupt.
			lastrpmcheck=millis; // We just checked!
			count=0; //Reset counter.
			if(rpm > 1000){ // Debug LED.
				PORTD |= (1<<PD7);
			}else{
				PORTD &=~(1<<PD7);
			}

			itoa(rpm,buf,10); //Turn int into string
			strcat(buf,"\r\n");//Add a newline
			print();//Tell the world!
		}
		//Counting up and down
		if(OCR1B > 254){
			addor=-1;
		}
		if(OCR1B < 10){
			addor=1;
		}
		
		OCR1B+=addor; // Do the addition.

		_delay_ms(10); // Fans aren't instant!
	}

		
}


//Millis ISR
ISR(TIMER0_COMP_vect){
	millis++;
}

//Pin Change ISR.
ISR(INT0_vect){
//We get two lows per rotation. Only want to count one.
 if(rot==0){
 	rot=1;
 	count++;
 	//PORTD ^=(1<<PD7);
 }else{
 	rot=0;
 }
}

//Serial xmit complete.
ISR(USART_UDRE_vect){
	if(finished==0){
	if(curpos>=buflen-1){
		finished=1;
		//Finished reading.
	}else{
		UDR=buf[curpos]; //Add a new byte
		curpos++;
	}

	}
}