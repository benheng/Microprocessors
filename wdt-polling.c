/* 
 * Initially the application blinks the red LED at a fixed speed (~0.25 seconds).
 * Each time the button is pressed the LED blinking rate responds depending on
 * when the button is pressed. When the light is on while the button is pressed, 
 * it will increase the blinking interval by ~0.25 seconds. When it is off while
 * the button is pressed, it will reset the blinking interval back to the default:
 * ~0.25 seconds. 
 * 
 * This example uses the Watchdog Timer (WDT) in interval mode to poll the button
 * and to count out the time interval for the blinks.
 * 
 * Launchpad hardware connections used for port 1:
 * P1.0: connected to RED LED (high turns on LED)
 * P1.6: connected to the GREEN LED (high turns on LED)
 * P1.3: connected to button (pressed connects to ground)
 */

// the following line loads #defines for MSP430 registers, etc.
#include  "msp430g2553.h"
// define bit masks for the 3 I/O channels that we use
#define RED 1  	// 0000 0001
#define GREEN 64	// 0100 0000
#define BUTTON 8	// 0000 1000

/* ===== GLOBAL VARIABLES =====
 * These variables are shared between the main program and the WDT
 * interrupt handler and represent the 'state' of the application.
 * blink_interval is made a variable rather than a constant so we can
 * change it in the debugger.
 */

unsigned int blink_interval;  			// number of WDT interrupts per blink of LED
volatile unsigned int blink_counter;   	// down counter for interrupt handler
unsigned char last_button_state;   		// state of the button the last time it was read
unsigned char red_led_state;			// state of the RED LED

// ===== Main Program (System Reset Handler) =====
void main(void)
{
  // setup the watchdog timer as an interval timer
  WDTCTL =(WDTPW + 		// (bits 15-8) password
                   		// bit 7=0 => watchdog timer on
                 	 	// bit 6=0 => NMI on rising edge (not used here)
                   	    // bit 5=0 => RST/NMI pin does a reset (not used here)
           WDTTMSEL + 	// (bit 4) select interval timer mode
           WDTCNTCL +  	// (bit 3) clear watchdog timer counter
  		          0 	// bit 2=0 => SMCLK is the source
  		          +1 	// bits 1-0 = 01 => source/8K ~ 134 Hz <-> 7.4ms.
  		   );
  IE1 |= WDTIE;			// enable the WDT interrupt (in the system interrupt register IE1)

  // initialize the I/O port.
  P1DIR = RED + GREEN;	// Set P1.0, P1.6, P1.3 is input (the default)
  P1REN = BUTTON;  		// enable internal 'PULL' resistor for the button
  P1OUT = BUTTON;		// pull up

  // initialize the state variables
  blink_interval=34; 	// corresponds to about 1/4 sec
  blink_counter=blink_interval;
  red_led_state = 0;	// LED starts off
  last_button_state=(P1IN&BUTTON);	// checks if the button is pressed on bootup

 // in the CPU status register, set bits:
 //    GIE == CPU general interrupt enable
 //    LPM0_bits == bit to set for CPUOFF (which is low power mode 0)
 _bis_SR_register(GIE+LPM0_bits);  // after this instruction, the CPU is off!
}

// ===== Watchdog Timer Interrupt Handler =====
// This event handler is called to handle the watchdog timer interrupt,
//    which is occurring regularly at intervals of 8K/1.1MHz ~= 7.4ms.

interrupt void WDT_interval_handler(){
  char current_button;           	// variable to hold the state of the button
  // poll the button to see if we need to change state
  current_button=(P1IN & BUTTON); 	// read button bit
  
  // This section handles the GREEN LED. ==============================================
  if (current_button) {				// check if the button is pressed (pressed = 0)
	  P1OUT &= ~GREEN;				// turn off when not pressed (AND ~GREEN mask)
  } else {
	  P1OUT |= GREEN;				// turn on when pressed (OR GREEN mask)
  }
  
  // This section handles the RED LED. ================================================
  if ((current_button==0) && last_button_state){ // did the button go down?
	  if (red_led_state){			// was the button pressed while the red LED was on?
		  blink_interval += 34;		// if so, increase blink delay by ~1/4 second
	  } else {						// else, reset blink interval
		  blink_interval = 34;		// corresponds to ~1/4 second
	  }
	  blink_counter=blink_interval;	// restart the counter
  }
  
  last_button_state=current_button; // remember the new button state
  
  // This section handles the blinking feature. =======================================
  if (--blink_counter==0){			// decrement the counter
	  P1OUT ^= RED; 				// when counter == 0, toggle LED
	  if (P1OUT & RED) {			// is the red LED on?
		  red_led_state = 1;
	  } else {
		  red_led_state = 0;
	  }
	  blink_counter=blink_interval; // reset the down counter
  }
}

// DECLARE function WDT_interval_handler as handler for interrupt 10
// using a macro defined in the msp430g2553.h include file
ISR_VECTOR(WDT_interval_handler, ".int10")

