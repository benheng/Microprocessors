/*
** Reads some sort of analog signal, convert it
** to digital and send it to the PC using UART.
** UART will be run at 9600 baud using the SMCLK.
**
** Baud rate divider with 1MHz = 1MHz/9600 = ~104.2
*/

#include "msp430g2553.h"
#include <stdio.h>

// GLOBAL VARIABLES
volatile unsigned int latest_adc_result;
volatile unsigned int timer_count = 0;
char buffer[32];

// FUNCTION PROTOTYPES
void init_led();
void init_uart();
void init_timer();
void init_adc();
void start_conversion();

// MAIN FUNCTION
void main(void) {
  // clock initializers
	WDTCTL 	 = WDTPW + WDTHOLD;	// stop the WDT
	BCSCTL1  = CALBC1_1MHZ;	 	// calibration for basic clock system
	DCOCTL 	 = CALDCO_1MHZ;	 	// calibration for digitally controlled oscillator

	// initiate peripherals
	init_led();
	init_uart();
	init_timer();
	init_adc();

	// enable interrupts and put the CPU to sleep
	_bis_SR_register(GIE+LPM0_bits);
}

// HELPER FUNCTIONS
void init_led() {
	P1DIR = 0x01 + 0x40;
}

void init_uart() {
	P1SEL  = BIT1 + BIT2;	// P1.1 = RXD, P1.2 = TXD
	P1SEL2 = BIT1 + BIT2;	// P1.1 = RXD, P1.2 = TXD
	UCA0CTL1 |= UCSSEL_2;	// SMCLK
	UCA0BR0 = 104;			// see baud rate divider above
	UCA0BR1 = 0;
	UCA0MCTL = UCBRS0;		// modulation UCBRSx = 1
	UCA0CTL1 &= ~UCSWRST;	// ** initialize USCI state machine **
	IE2 |= UCA0TXIE;		// Enable USCI_A0 TX interrupt
}

void init_timer() {
	TA0CTL |= TACLR;		// reset timer
	TA0CTL  = TASSEL_2		// SMCLK
			+ ID_0			// input divider = 1
			+ MC_2;			// continuous mode, interrupt disabled
	TA0CCTL0 = OUTMOD_2		// compare mode
			+ CCIE			// interrupt enabled
			+ CCIFG;
}

void init_adc() {
	// :NOTE: On-board heat sensor is used.
	ADC10CTL1 = INCH_10		// temperature sensor input to ADC
			+ SHS_0			// use ADC10SC bit to trigger sampling
			+ ADC10DIV_3	// clock divider = 4
			+ ADC10SSEL_3	// clock source = SMCLK
			+ CONSEQ_0;		// single channel, single conversion
	ADC10DTC1 = 1;			// one block per transfer

	ADC10CTL0 = SREF_1		// reference voltages are Vss and Vcc
			+ ADC10SHT_3	// 64 ADC10 clocks for sample and hold time (slowest)
			+ REFON			// reference generator on
			+ ADC10ON		// turn on ADC10
			+ ENC;			// enable (but not yet start) conversions
}

void start_conversion() {
	if ((ADC10CTL1 & ADC10BUSY) == 0) {	// if not already converting
		P1OUT ^= 0x40;
		ADC10CTL0 |= ADC10SC;
		ADC10SA = (unsigned) &latest_adc_result;
	}
}

// INTERRUPT HANDLERS
#pragma vector = TIMER0_A0_VECTOR
__interrupt void Timer0_A0(void) {
	timer_count++;
	if (timer_count > 16) {
		timer_count = 0;
		start_conversion();
		IE2 |= UCA0TXIE;
	}
}

#pragma vector = USCIAB0TX_VECTOR
__interrupt void USCI0TX_ISR(void) {
	P1OUT |= 0x01;
	unsigned int i = 0;					// iterator pointers
	sprintf(buffer, "temp: %d F\n\r", (int)(latest_adc_result/10));

	while (buffer[i] != '\0') {
		while (!(IFG2 & UCA0TXIFG));	// USCI_A0 TX buffer ready?
		UCA0TXBUF = buffer[i++];
	}

	P1OUT &= ~0x01;
	IE2 &= ~UCA0TXIFG;
}
