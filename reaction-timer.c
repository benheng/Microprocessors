/*
 * Reaction Timer
 *  1. Waits for the user to press the button.
 * 	2. (optional) Wait a short "random" amount of time.
 * 	3. Flash the green light 2 times and then the red light once.
 * 	4. As soon as the user sees the red light go off, s/he should press the button again.
 * 		The application should estimate HOW LONG it took between the red light going
 * 		off and the user pressing down on the button. Store that time in a global variable
 * 		that can be read in the debugger.
 * 	5. Repeat.
 *
 */

#include "msp430g2553.h"

// Define bit masks for the I/O channels in use
#define RED 1		// 0000 0001
#define GREEN 64	// 0100 0000
#define BUTTON 8	// 0000 1000

#define BLINK_INTERVAL 1000

// GLOBAL VARIABLES
volatile unsigned int ms_timer;		// each int has been calibrated to equal 1ms
unsigned int last_time;				// keep the score of the last reaction time
unsigned int init_countdown;		// initialize the count down
unsigned int init_timer;			// initialize the timer
unsigned int stop_timer;			// stop the timer
unsigned char last_button_state;	// state of the button the last time it was read

// ===== Main Program (System Reset Handler) =====
void main(void)
{
	// Initial clock calibration
	BCSCTL1 = CALBC1_8MHZ;
	DCOCTL	= CALDCO_8MHZ;

	// Set up the watchdown timer as an interval timer
	WDTCTL = (WDTPW +		// (bits 15-8) password
							// bits 7=0 => watchdog timer on
							// bit 6=0 => NMI on rising edge (not used here)
							// bit 5=0 => RST/NMI pin does a reset (not used here)
				WDTTMSEL +	// (bit 4) select interval timer mode
				WDTCNTCL +	// (bit 3) clear watchdog timer counter
							// bit 2=0 => SMCLK is the source
				1			// bits 1-0 = 01 => source/8K = 8MHz/8K = 1KHz <-> ~1ms.
				);
	IE1 |= WDTIE;			// enable the WDT interrupt (in the system interrupt register IE1)

	// Initialize the I/O port
	P1DIR = RED + GREEN;
	P1REN = BUTTON;
	P1OUT = BUTTON;

	// Initialize state variables
	ms_timer = 0;							// initialize the time to 0ms
	last_time = 0;							// reset the score
	init_countdown = 0;						// don't start the count down
	init_timer = 0;							// don't start the timer
	stop_timer = 1;							// stop the timer
	last_button_state = (P1IN & BUTTON);	// check button press on bootup

	// Set CPU status register:
	//	GIE == CPU general interrupt enable
	//	LPM0_bits == bit to set for CPUOFF (which is low power mode 0)
	_bis_SR_register(GIE+LPM0_bits);  // after this instruction, the CPU is off!
}

// ===== Watchdog Timer Interrupt Handler =====
interrupt void WDT_interval_handler()
{
	char current_button;							// Holds the current button state
	current_button = (P1IN & BUTTON);				// Poll the button to see if it's changed

	if ((current_button==0) && last_button_state) {	// Did the button go down?
		if (!init_countdown && !init_timer)			// If nothing was initialized
		{
			init_countdown = 1;						// Initialize the count down
			P1OUT &= ~RED;							// Turn off RED LED
		}
		if (init_timer)								// If the main timer is initialized
			stop_timer = 1;							// Stop the main timer
	}
	last_button_state = current_button;				// Remember the new button state

	// Count down initialized: blink the GREEN LED twice and the RED LED once.
	if (init_countdown == 1 || init_countdown == 3)
	{
		if (++ms_timer == BLINK_INTERVAL)
		{
			ms_timer = 0;
			init_countdown++;
			P1OUT |= GREEN;
		}
	}
	else if (init_countdown == 2 || init_countdown == 4)
	{
		if (++ms_timer == BLINK_INTERVAL/10)
		{
			ms_timer = 0;
			init_countdown++;
			P1OUT &= ~GREEN;
		}
	}
	else if (init_countdown == 5)
	{
		if (++ms_timer == BLINK_INTERVAL)
		{
			ms_timer = 0;
			init_countdown++;
			P1OUT |= RED;
		}
	}
	else if (init_countdown == 6)
	{
		if (++ms_timer == BLINK_INTERVAL/10)
		{
			ms_timer = 0;
			init_countdown = 0;
			P1OUT &= ~RED;
			init_timer = 1;
			stop_timer = 0;
		}
	}
	
	// Reaction timer initialized: count the milliseconds
	if (init_timer)
	{
		if (stop_timer)
		{
			last_time = ms_timer;
			ms_timer = 0;
			init_timer = 0;
			P1OUT |= RED;
		}
		else
			ms_timer++;
	}
}

// Declare function WDT_interval_handler as handler for interrupt 10
// using a macro defined in the msp430g2553.h include file
ISR_VECTOR(WDT_interval_handler, ".int10")
