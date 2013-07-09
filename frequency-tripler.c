/*
** Frequency Tripler
*/

#include "msp430g2553.h"

// DEFINES
#define TA0_BIT 0x02  // P1.1 TA0 Out0 (output)
#define TA1_BIT 0x04	// P1.2 TA0 CCI1A (input)

// GLOBAL VARIABLES
volatile unsigned int curr_edge_time = 0;
volatile unsigned int prev_edge_time = 0;
volatile unsigned int half_period = 0;

// MAIN PROGRAM (SYSTEM RESET HANDLER)
void main(void)	// only one .c file may contain main
{
	// clock initializers
	WDTCTL 	 = WDTPW + WDTHOLD;	// stop the WDT
	BCSCTL1  = CALBC1_16MHZ;	// calibration for basic clock system
	DCOCTL 	 = CALDCO_16MHZ;	// calibration for digitally controlled oscillator

	// TACC0 initializer (generates tripled frequency)
	TACCTL0  = OUTMOD_4 + CCIE;	// toggle output, compare mode, interrupt enabled
	TACCR0	 = half_period;		// initial half period
	P1SEL	|= TA0_BIT;			// connect TACC0 to P1.1
	P1DIR	|= TA0_BIT;			// set P1.1 to output

	// TACC1 initializer (captures input frequency)
	TACCTL1  = (CM_1 +			// capture on rising edge
				CCIS_0 +		// CC input select (CCI1A)
				SCS +			// synchronous capture
	//			SCCI +			// synchronized CC input
				CAP +			// capture mode
				CCIE);			// interrupt enabled
	P1SEL	|= TA1_BIT;			// connect TACC1 to P1.2 as an input

	// timer0_A initializers
	TACTL 	|= TACLR;			// reset TIMER_A
	TACTL 	 = (TASSEL_2 + 		// clock source: SMCLK
				    ID_0 + 		// input divider: 1
				    MC_2);		// continuous mode (interrupt disabled)

	// enable interrupts and put the CPU to sleep
	_bis_SR_register(GIE+LPM0_bits);
}

// INTERRUPT HANDLERS
void interrupt frequency_handler() {
	TACCR0 += half_period;		// (calculations done when input is read)
}
ISR_VECTOR(frequency_handler,".int09")

void interrupt reader_handler() {
	switch (TAIV) {
		case 2:
			curr_edge_time = TACCR1;
			if (prev_edge_time < curr_edge_time) {	// only if TAR doesn't reset (overflow)
				// assuming 50% duty cycle, we only take measurements at rising edge
				// (curr_edge - prev_edge)/2 = input_half_period
				// input_half_period/3 = tripled_freq_half_period
				// (curr_edge - prev_edge)/6 = tripled_freq_half_period
				half_period = (curr_edge_time - prev_edge_time)/6;
			}
			prev_edge_time = curr_edge_time;
			break;
	}
}
ISR_VECTOR(reader_handler,".int08")
