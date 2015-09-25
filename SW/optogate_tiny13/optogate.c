// Program written for ATmega8
#define F_CPU 4800000
#include <avr/io.h>
#include <util/delay.h>
#include <stdlib.h>
#include <avr/interrupt.h>
#include <avr/eeprom.h>

#define ledon() PORTB|=8
#define ledoff() PORTB&=~8

#define IRon() PORTB|=4
#define IRoff() PORTB&=~4

#define sinetoggle() PORTB^=16

volatile char sine_enable = 0;
volatile char IR_counter =0;
volatile char state = 1;
volatile char muted = 0;


void initTimr3(){
	TCCR0A = (1<<WGM01 ) | (1<<WGM00 ); // Fast PWM
	TCCR0B = (1<<WGM02 ) | (1<<CS00 );	// Fast PWM, no prescaler
	OCR0A = 118; 						// comapre register A: top of counter : 118 for 19.5kHz sine out @ 4.8Mhz clk
	TIMSK0 = (1<<OCIE0A);				// compare A ineterrupt enable
}

void initTimr(){	//PB0 a kimenet, 4,8MHz kvarc mellett 19.6 kHz kimenõ jel
	TCCR0A =   (1<<WGM00 )|(1<< COM0A0)  ; //  PWM , OC0A toggle
	TCCR0B = (1<<WGM02 ) | (1<<CS00 );	//  PWM, no prescaler
	OCR0A = 59; 					// comapre register A: top of counter : 118 for 19.5kHz sine out @ 4.8Mhz clk
//	TIMSK0 = (1<<OCIE0A);				// compare A ineterrupt enable
}
void initTimr2(){	//PB1 a kimenet, 4,8MHz kvarc mellett 19.6 kHz kimenõ jel
	TCCR0A =   (0<<WGM00 )| (0<<WGM01 )|(1<< COM0B0)|(0<< COM0B1)|(0<< COM0A0)|(0<< COM0A1)  ; //  PWM , OC0A toggle
	TCCR0B = (0<<WGM02 ) | (0<<CS00 );	//  PWM, no prescaler, init value is OFF timer
	OCR0B = 98; 					// comapre register A: top of counter : 118 for 19.5kHz sine out @ 4.8Mhz clk
	TIMSK0 = (1<<OCIE0B);				// compare A ineterrupt enable
}

void signal_on(){
//	TCCR0B = (0<<WGM02 ) | (1<<CS00 );// for 2
TCCR0B = (1<<WGM02 ) | (1<<CS00 ); // for 1

ledon();
}

void signal_off(){
//	TCCR0B = (0<<WGM02 );// for 2
TCCR0B = (1<<WGM02 ) | (0<<CS00 ); // for 1

ledoff();

}

void initComparator(){
	ADCSRB = (1<<ACME); 	// ADC off
	ADMUX = (1<<MUX1) | (0<<MUX0);// AIN1 is the negative input
	ACSR |= 1<<ACIE;		// interrupt enable

	DIDR0= (1<<AIN0D );		// disable digital
}

ISR (ANA_COMP_vect){
//	sine_enable= 255;
//	ledon();
//_delay_ms(1000);
}


ISR (TIM0_COMPB_vect ){	//HACK to get OC0B pwm out: //OC0B csak FF értékére vált állapotot, de nekem hamarabb kell, compare match esetén átállítom a timert, hogy váltson :)
//TCNT0 =255;

}


ISR (TIM0_OVF_vect  ){
//TCNT0 =255-59;
//ledon();
}

/*
ISR (TIM0_COMPA_vect ){

	if(sine_enable){
		sinetoggle();
		ledon();
		sine_enable--;
	}
	else{
		ledoff();
	}
	
		
	if(IR_counter>=200){
		IRon();
	}
	else{
		IRoff();
	}

	IR_counter++;



}*/

void Mute_on(void){
	muted=1;
	DDRB|= 2;
	PORTB|=2;
}

void Mute_off(void){
	muted=0;
	/**/
	PORTB&=!2;
	_delay_ms(1);
	/**/
	PORTB&=!2;
	DDRB= 8 |4|1;
	
}




uint8_t Get_state(void){
	uint8_t  byteRead = eeprom_read_byte((uint8_t*)1); 
	return byteRead;
}

void Set_state(uint8_t data){
	eeprom_write_byte ((uint8_t*) 1, data); 
}


void Blink(char num){
	for (int i=0; i<num; i++){
		for(int j=0; j<5; j++){
			ledon();
			_delay_ms(30);
			ledoff();
			_delay_ms(30);
			}
		_delay_ms(500);
		}
	_delay_ms(500);
}




ISR (PCINT0_vect ){
if(sine_enable<10)
	sine_enable+=2;
//sine_enable=1;
//signal_on();
//_delay_ms(1000);

}

void initExtint(){
	PCMSK=(1<<PCINT4 ); // pcint4 on pb4 enable
	GIFR = (1<<PCIF);	//enable pin change int
	GIMSK =(1<<PCIE);	//PCint int en
}

int main(){




DDRB= 8 |4|1;
initTimr();
initExtint();
//initTimr2();
//initComparator();
sei();
sine_enable=1;


state= Get_state();
if(state==0)
	state=1;
//state=1;
//Blink(state);


while(1){
//SEND SIGNAL IF SOMEONE IS HERE//////////////////////
	if(state==1){
		if(sine_enable>0){
			sine_enable--;
		}
		IRon();
		_delay_us(10);
		IRoff();
		_delay_ms(100);
		if(sine_enable>5){
			ledon();
			//sine_enable=0;
		}
		else{
			ledoff();
		}
	}

//MUTE IF NOBODY IS HERE//////////////////////////////
	if(state==2){
		if(sine_enable>0){
			sine_enable--;
		}
		IRon();
		_delay_us(10);
		IRoff();
		_delay_ms(100);
		if(sine_enable>5){
			Mute_off();
			ledon();
			//sine_enable=0;
		}
		else{
			Mute_on();
			ledoff();
		}
	}


//DO NOTHING /////////////////////////////////////////
	if(state==3){
		ledoff();
		Mute_off();
		_delay_ms(50);
	}



/*  STATE CHANGE */

if(muted ==0){
	if(PINB & 2){
		state++;
		if(state >3) state =1;
		Blink (state);
		Set_state(state);
	}
}











}//end of while











 return 0;
}
