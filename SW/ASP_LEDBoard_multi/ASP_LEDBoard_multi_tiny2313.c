#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>

/***********************SETTINGS***************/
// a 3 közül csak egy lehet igaz
#define ASP_LED_BOARD_100			0		// ventillátoros BSS automatán választaj a ki a mûködést: 5V vagy 12V	
#define ASP_LED_BOARD_50_2_LED		1		// ventillátor nélküli BSS 2 led, nincs relé
#define ASP_LED_BOARD_50_4_LED		0		// ventillátor nélküli BSS 4 led, van relé

/***********************SETTINGS***************/

#define FAN_SPEED 10

#define LED3_ON 	PORTD|=2
#define LED3_OFF	PORTD&=~2
#define LED2_ON 	PORTD|=1
#define LED2_OFF	PORTD&=~1
#define LED1_ON 	PORTA|=2
#define LED1_OFF	PORTA&=~2
#define LED0_ON 	PORTD|=32
#define LED0_OFF	PORTD&=~32
#define LED4_ON 	PORTD|=64
#define LED4_OFF	PORTD&=~64

#define RELAY1_ON 	PORTD|=8
#define RELAY1_OFF	PORTD&=~8
#define RELAY2_ON 	PORTA|=1
#define RELAY2_OFF	PORTA&=~1

#define BUTTON 		(PIND & 4)
#define PROC_SIGNAL (PINB & 2)
#define MODE_12V 	(PIND & 16)
#define MODE_5V 	!MODE_12V

#define FAN1_H		(PINB & 128)
#define FAN1_L		!FAN1_H
#define FAN2_H		(PINB & 64)
#define FAN2_L		!FAN2_H

volatile char FAN1H_state=0;
volatile char FAN1L_state=0;
volatile char FAN2H_state=0;
volatile char FAN2L_state=0;

volatile int timer_divider=0;
volatile char LED0_blink=0;
volatile char LED1_blink=0;
volatile char LED2_blink=0;
volatile char LED3_blink=0;
volatile char LED4_blink=0;

volatile char LED_blink_state=0;

#define power_down		0
#define switching_on	1
#define switching_off	2
#define power_up		3
volatile char sytem_state=0;


char IRQ_handle(void){
		if(sytem_state==power_down){
		sytem_state=switching_on;
		LED4_ON;
		RELAY2_OFF; LED3_OFF; 
		RELAY1_ON; LED2_ON;
		return 0;
		}
	if(sytem_state==switching_on ){
		sytem_state=power_down;
		LED4_OFF;
		RELAY1_OFF; LED2_OFF;
		RELAY2_OFF; LED3_OFF;
		return 0;
		}
	if(sytem_state==power_up){
		sytem_state=switching_off;
		LED4_OFF;
		RELAY1_ON; LED2_ON;
		RELAY2_OFF; LED3_OFF;
		return 0;
		}
	if(sytem_state==switching_off ){
		sytem_state=power_up;
		LED4_ON;
		RELAY1_ON; LED2_ON;
		RELAY2_ON; LED3_ON;
		return 0;
		}
	
	_delay_ms(5);
}


 //***********************************************************************
ISR (INT0_vect){
	if(MODE_12V||ASP_LED_BOARD_50_4_LED) IRQ_handle();	//csak akkor kezelek le gombot, ha van 12V az ASP_100-on, vagy 4 LEDES az ASP_50
	
}

void IntInit(void){
	MCUCR= 1<<ISC01;	// falling level on int0
	GIMSK= 1<<INT0;		//enable int0 		
}

void PWMInit(void){
	TCCR1A=1<<COM1A1 | 1<<WGM10; ;	// OC1A: set at top, clr at match
	TCCR1B=  1<<CS10 | 1<<WGM12; //div8 //1<<CS11 |
	OCR1AH=0;
	OCR1AL=0; //alapállapotban kikapcsoljuk a ventillátort

}

void Timer0Init(void){
	TCCR0A=0;			// normal operation no pwm
	TCCR0B= 1<<CS00 ;	// no prescaler
	TIMSK= 1<<TOIE0;	//ovf int enable


}

ISR (TIMER0_OVF_vect ){
	timer_divider++;
	if(timer_divider==12000){
		timer_divider=0;
		//ledek villogtatása
		if(LED0_blink && !LED_blink_state) LED0_ON;
		if(LED0_blink && LED_blink_state) LED0_OFF;
		if(LED1_blink && !LED_blink_state) LED1_ON;
		if(LED1_blink && LED_blink_state) LED1_OFF;
		if(LED2_blink && !LED_blink_state) LED2_ON;
		if(LED2_blink && LED_blink_state) LED2_OFF;
		if(LED3_blink && !LED_blink_state) LED3_ON;
		if(LED3_blink && LED_blink_state) LED3_OFF;
		if(LED4_blink && !LED_blink_state) LED4_ON;
		if(LED4_blink && LED_blink_state) LED4_OFF;
		LED_blink_state=!LED_blink_state;
	
	}
	//vantillátor mûkösésének ellenõrzése
	if(FAN1_H) FAN1H_state=10;
	if(FAN1_L) FAN1L_state=10;
	if(FAN2_H) FAN2H_state=10;
	if(FAN2_L) FAN2L_state=10;
}



//***********************************************************************
int main(){
	//Portinit
	DDRA=0b00000011;
	DDRB=0b00001000;
	DDRD=0b01101011;

	IntInit();
	PWMInit();
	Timer0Init();
	sei();

while(1){	//fõciklus
	
	 LED0_blink=0;
	 LED1_blink=0;
	 LED2_blink=0;
	 LED3_blink=0;
	 LED4_blink=0;
	 LED0_OFF;
	 LED1_OFF;
	 LED2_OFF;
	 LED3_OFF;
	 LED4_OFF;
	 RELAY1_OFF;
	 RELAY2_OFF;

	while((MODE_12V && ASP_LED_BOARD_100) || ASP_LED_BOARD_50_4_LED){
					

		//FAN test
		if(!sytem_state==power_down)
			if((FAN1H_state & FAN1L_state & FAN2H_state & FAN2L_state) || ASP_LED_BOARD_50_2_LED || ASP_LED_BOARD_50_4_LED) {
				LED0_blink=0;
				LED0_ON;
				OCR1AL=FAN_SPEED;
				
			}
			else{
				OCR1AL=255;
				LED0_blink=1;
		}
			
			 
		if(sytem_state==power_down) {
			OCR1AL=0;
			LED0_blink=0;
			LED0_ON;
		}


			if(FAN1H_state) FAN1H_state--;
			if(FAN1L_state) FAN1L_state--;
			if(FAN2H_state) FAN2H_state--;
			if(FAN2L_state) FAN2L_state--;


		//uC test
		if(PROC_SIGNAL && !sytem_state==power_down){
			LED1_blink=0;
			LED1_ON;
		}

		if(!PROC_SIGNAL && !sytem_state==power_down){
			LED1_blink=1;
			//LED1_ON;
		}

		if( sytem_state==power_down){
			LED1_blink=0;
			LED1_OFF;
		}


		//relék kapcsolása
		if(sytem_state==switching_on ){
			_delay_ms(1000);
			_delay_ms(1000);
			_delay_ms(1000);
			_delay_ms(1000);
			_delay_ms(1000);
			if(sytem_state==switching_on ){
				sytem_state=power_up;
				LED4_ON;
				RELAY1_ON; LED2_ON;
				RELAY2_ON; LED3_ON;
			}
		}

		if(sytem_state==switching_off ){
			_delay_ms(1000);
			_delay_ms(1000);
			_delay_ms(1000);
			_delay_ms(1000);
			_delay_ms(1000);
			if(sytem_state==switching_off ){
				sytem_state=power_down;
				LED4_OFF;
				RELAY1_OFF; LED2_OFF;
				RELAY2_OFF; LED3_OFF;
			}
		}
		_delay_ms(1);
		
	}//end of MODE_12V

	 LED0_blink=0;
	 LED1_blink=0;
	 LED2_blink=0;
	 LED3_blink=0;
	 LED4_blink=0;
	 LED0_OFF;
	 LED1_OFF;
	 LED2_OFF;
	 LED3_OFF;
	 LED4_OFF;
	 RELAY1_OFF;
	 RELAY2_OFF;

	while((MODE_5V&&ASP_LED_BOARD_100) || ASP_LED_BOARD_50_2_LED){

		//FAN test
		if((FAN1H_state & FAN1L_state & FAN2H_state & FAN2L_state) || ASP_LED_BOARD_50_2_LED || ASP_LED_BOARD_50_4_LED) {
			LED1_blink=0;
			LED1_ON;
			OCR1AL = FAN_SPEED;
			//_delay_ms(1000);
			
		}
		else{
			LED1_blink=1; 
			OCR1AL=255;
			_delay_ms(1);
			
		}


			if(FAN1H_state) FAN1H_state--;
			if(FAN1L_state) FAN1L_state--;
			if(FAN2H_state) FAN2H_state--;
			if(FAN2L_state) FAN2L_state--;


		//uC test
		if(PROC_SIGNAL){
			LED2_blink=0;
			LED2_ON;
		}
		else LED2_blink=1;

		_delay_ms(1);
	}//end of MODE_5V
}//while(1)	
}//end of main
