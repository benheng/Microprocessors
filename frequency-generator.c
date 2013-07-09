/*
** Frequency Generator
*/

#include "msp430g2553.h"

// DEFINES
#define TA0_BIT 0x02  // P1.1 TA0 output
#define BUTTON 0x08		// P1.3 Button

// GLOBAL VARIABLES
volatile int next_state = 0;	// next_state variable
volatile int on_off = 0;		// 0 or OUTMOD_4 (ox0080)
volatile int half_period = 0;	// range = 250Hz to 10KHz

// MAIN PROGRAM (SYSTEM RESET HANDLER)
void main(void)	// only one .c file may contain main
{
	// clock initializers
	WDTCTL 	 = WDTPW + WDTHOLD;	// stop the WDT
	BCSCTL1  = CALBC1_16MHZ;	// calibration for basic clock system
	DCOCTL 	 = CALDCO_16MHZ;	// calibration for digitally controlled oscillator

	// timer initializers
	TACTL 	|= TACLR;			// reset TIMER_A
	TACTL 	 = (TASSEL_2 + 		// clock source: SMCLK
				    ID_0 + 		// input divider: 1
				    MC_2);		// continuous mode (interrupt disabled)
	TACCTL0  = on_off + CCIE;	// TACCTL0 generates notes, interrupt enabled
	TACCR0	 = half_period;		// initial period
	P1SEL	|= TA0_BIT;			// connect timer to P1.1
	P1DIR	|= TA0_BIT;			// set P1.1 to output

	// button initializers
	P1OUT 	|= BUTTON;			// set P1.3 output = 1
	P1REN 	|= BUTTON;			// enable PULLUP resistor
	P1IES	|= BUTTON;			// set falling edge
	P1IFG	&= ~BUTTON;			// clear interrupt flag
	P1IE	|= BUTTON;			// enable interrupt

	// enable interrupts and put the CPU to sleep
	_bis_SR_register(GIE+LPM0_bits);
}

// INTERRUPT HANDLERS
void interrupt frequency_handler() {
	TACCR0 += half_period;		// advance 'alarm' time
	TA0CCTL0 = on_off + CCIE;	// frequency on or off
}
ISR_VECTOR(frequency_handler,".int09")

void interrupt button_handler() {
	if (P1IFG & BUTTON) {		// no debouncing
		on_off = OUTMOD_4;
		switch (next_state) {
			case 0:
				half_period = 32000;	// 250Hz
				break;
			case 1:
				half_period = 16000;	// 500Hz
				break;
			case 2:
				half_period = 8000;		// 1KHz
				break;
			case 3:
				half_period = 4000;		// 2KHz
				break;
			case 4:
				half_period = 1600;		// 5KHz
				break;
			case 5:
				half_period = 800;		// 10KHz
				break;
			default:
				on_off = 0;
				next_state = -1;
		}
		next_state++;
		P1IFG &= ~BUTTON;	// reset the interrupt flag
	}
}
ISR_VECTOR(button_handler,".int02")
