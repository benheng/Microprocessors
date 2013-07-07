/* INCOMPLETE, TODO: change it's operation so the cpu isn't on doing nothing (i.e. make it sleep)
** Music Player
**   This program uses Timer_A (TACTL) and two of its comparators, TACCTL0 and TACCTL1. There are
** 	two interrupt handlers, one for each comparator. TACCTL0 toggles the output P1.1 and drives
** 	a speaker while TACCTL1 times the duration in order to space out the notes to play
** 	programmable songs.
**
** NOTE: The CPU does NOT go to sleep and the button press does NOT trigger an interrupt. If the
** 	song sequence was triggered by an interrupt, then the other handlers that handle toggling
** 	the output to produce the note and timing the duration never get executed. The MSP430 can
** 	only handle one interrupt request at a time based on priority.
*/

#include "msp430g2553.h"

// MUSIC PLAYER DEFINITIONS
// 	define notes: 1,000,000 Hz / note frequency / 2
//  note frequency values obtained from http://www.phy.mtu.edu/~suits/notefreqs.html
// 	subtract 1 from the final answer to shift notes from [1, half_period] to
// 	[0, half_period-1]
#define SILENCE_MS 20
#define C3 3821
#define Db3 3607
#define D3 3404
#define Eb3 3213
#define E3 3033
#define F3 2863
#define Gb3 2702
#define G3 2550
#define Ab3 2407
#define A3 2272
#define Bb3 2144
#define B3 2024
#define C4 1910
#define Db4 1803
#define D4 1702
#define Eb4 1606
#define E4 1516
#define F4 1431
#define Gb4 1350
#define G4 1275
#define Ab4 1203
#define A4 1135
#define Bb4 1072
#define B4 1011
#define C5 955
#define Db5 901
#define D5 850
#define Eb5 803
#define E5 757
#define F5 715
#define Gb5 675
#define G5 637
#define Ab5 601
#define A5 567
#define Bb5 535
#define B5 505
#define C6 477
#define Db6 450
#define D6 425
#define Eb6 401
#define E6 378
#define F6 357
#define Gb6 337
#define G6 318
#define Ab6 300
#define A6 283
#define Bb6 267
#define B6 252
#define C7 238
#define Db7 224
#define D7 212
#define Eb7 200
#define E7 189
#define F7 178
#define Gb7 168
#define G7 158
#define Ab7 150
#define A7 141
#define Bb7 133
#define B7 126
#define C8 118
#define Db8 112
#define D8 105
#define Eb8 99

// DEFINE I/O
#define TA0_BIT 0x02	// 0000 0010
#define BUTTON 0x08		// 0000 1000

// GLOBAL VARIABLES
volatile unsigned int play = 0;
volatile unsigned int sound_enabled = 0;
volatile unsigned int current_note = C6;
volatile unsigned int ms_duration = 0;
volatile unsigned int ms_elapsed = 0;
volatile unsigned int ms_per_tick = 60000 / (4 * 160);
// to calculate ms_per_tick: 60000ms / (ticks_per_beat * beats_per_minute)

// FUNCTION PROTOTYPES
void play(unsigned int, unsigned int);
void rest(unsigned int);
void play_song();

// MAIN PROGRAM (SYSTEM RESET HANDLER)
void main(void)
{
	// clock initializers
	WDTCTL 	 = WDTPW + WDTHOLD;	// stop the WDT
	BCSCTL1  = CALBC1_1MHZ;		// 1MHz calibration for basic clock system
	DCOCTL 	 = CALDCO_1MHZ;		// 1MHz calibration for digitally controlled oscillator

	// timer initializers
	TACTL 	|= TACLR;			// reset TIMER_A
	TACTL 	 = (TASSEL_2 + 		// clock source: SMCLK
				    ID_0 + 		// input divider: 1
				    MC_2);		// continuous mode (interrupt disabled)
	TACCTL0  = CCIE;			// TACCTL0 generates notes, interrupt enabled
	TACCR0	 = current_note;	// initial note
	TACCTL1  = CCIE;			// TACCTL1 times durations, interrupt enabled
	P1SEL	|= TA0_BIT;			// connect timer to P1.1
	P1DIR	|= TA0_BIT;			// set P1.1 to output

	// button initializers
	P1OUT 	|= BUTTON;			// set P1.3 as an output
	P1REN 	|= BUTTON;			// enable PULLUP resistor
	P1IES	|= BUTTON;			// set falling edge
	P1IFG	&= ~BUTTON;			// clear interrupt flag
	P1IE	|= BUTTON;			// enable interrupt

	// enable interrupts
	_bis_SR_register(GIE);

	// play when the button is pressed
	while (1) {
		while (P1IN & BUTTON);
		play_song();
	}
}

// HELPER FUNTIONS
void measure_1() {
	play (A6, 1);
	play (D6, 1);
	play (Bb5, 1);
	play (G5, 1);
	play (G6, 1);
	play (D6, 1);
	play (Bb5, 1);
	play (G5, 1);
	play (Gb6, 1);
	play (D6, 1);
	play (Bb5, 1);
	play (G5, 1);
	play (G6, 1);
	play (D6, 1);
	play (Bb5, 1);
	play (G5, 1);
}
void measure_2() {
	play (G6, 1);
	play (C6, 1);
	play (A5, 1);
	play (F5, 1);
	play (F6, 1);
	play (C6, 1);
	play (A5, 1);
	play (F5, 1);
	play (E6, 1);
	play (C6, 1);
	play (A5, 1);
	play (F5, 1);
	play (F6, 1);
	play (C6, 1);
	play (A5, 1);
	play (F5, 1);
}
void measure_3() {
	play (F6, 1);
	play (Bb5, 1);
	play (G5, 1);
	play (E5, 1);
	play (E6, 1);
	play (Bb5, 1);
	play (G5, 1);
	play (E5, 1);
	play (Eb6, 1);
	play (Bb5, 1);
	play (G5, 1);
	play (E5, 1);
	play (E6, 1);
	play (Bb5, 1);
	play (G5, 1);
	play (E5, 1);
}
void measure_4() {
	play (E6, 1);
	play (A5, 1);
	play (F5, 1);
	play (D5, 1);
	play (D6, 1);
	play (A5, 1);
	play (F5, 1);
	play (D5, 1);
	play (Db6, 1);
	play (A5, 1);
	play (F5, 1);
	play (D5, 1);
	play (D6, 1);
	play (A5, 1);
	play (F5, 1);
	play (D5, 1);
}
void measure_5() {
	play (Bb6, 1);
	play (Eb6, 1);
	play (C6, 1);
	play (Gb5, 1);
	play (A6, 1);
	play (Eb6, 1);
	play (C6, 1);
	play (Gb5, 1);
	play (Ab6, 1);
	play (Eb6, 1);
	play (C6, 1);
	play (Gb5, 1);
	play (A6, 1);
	play (Eb6, 1);
	play (C6, 1);
	play (Gb5, 1);
}
void measure_6() {
	play (C7, 1);
	play (D6, 1);
	play (Bb5, 1);
	play (G5, 1);
	play (Bb6, 1);
	play (D6, 1);
	play (Bb5, 1);
	play (G5, 1);
	play (A6, 1);
	play (D6, 1);
	play (Bb5, 1);
	play (G5, 1);
	play (Bb6, 1);
	play (D6, 1);
	play (Bb5, 1);
	play (G5, 1);
}
void measure_7() {
	play (A6, 1);
	play (Bb5, 1);
	play (G5, 1);
	play (E5, 1);
	play (G6, 1);
	play (Bb5, 1);
	play (G5, 1);
	play (E5, 1);
	play (F6, 1);
	play (Bb5, 1);
	play (G5, 1);
	play (E5, 1);
	play (E6, 1);
	play (Bb5, 1);
	play (G5, 1);
	play (E5, 1);
}
void measure_10() {
	play (F5, 2);
	play (A5, 2);
	play (B5, 4);
	play (F5, 2);
	play (A5, 2);
	play (B5, 4);
	play (F5, 2);
	play (A5, 2);
	play (B5, 2);
	play (E6, 2);
	play (D6, 4);
	play (B5, 2);
	play (C6, 2);
}
void measure_11() {
	play (B5, 2);
	play (G5, 2);
	play (E5, 10);
	play (D5, 2);
	play (E5, 2);
	play (G5, 2);
	play (E5, 12);
}
void measure_12() {
	play (E6, 2);
	play (B5, 2);
	play (G5, 10);
	play (B5, 2);
	play (G5, 2);
	play (D5, 2);
	play (E5, 12);
}
void measure_13() {
	play (D4, 2);
	play (E4, 2);
	play (F4, 4);
	play (G4, 2);
	play (A4, 2);
	play (B4, 4);
	play (C5, 2);
	play (B4, 2);
	play (E4, 12);
}
void measure_14() {
	play (F5, 2);
	play (G5, 2);
	play (A5, 4);
	play (B5, 2);
	play (C6, 2);
	play (D6, 4);
	play (E6, 2);
	play (F6, 2);
	play (G6, 12);
}
void measure_15() {
	play (F5, 2);
	play (E5, 2);
	play (A5, 2);
	play (G5, 2);
	play (B5, 2);
	play (A5, 2);
	play (C6, 2);
	play (B5, 2);
	play (D6, 2);
	play (C6, 2);
	play (E6, 2);
	play (D6, 2);
	play (F6, 2);
	play (E6, 2);
	play (B5, 1);
	play (C6, 2);
	play (A5, 1);
}

void play_song()
{
	// plays programmable songs

	// Legend of Zelda - Lost Woods
	ms_per_tick = 60000 / (4 * 160);
	measure_10();
	measure_11();
	measure_10();
	measure_12();
	measure_13();
	measure_14();
	measure_13();
	measure_15();
	play (B5, 16);
	rest (16);

	// Legend of Zelda - Great Fairy Fountain
	ms_per_tick = 60000 / (4 * 80);
	measure_1();
	measure_2();
	measure_3();
	measure_4();
	measure_1();
	measure_5();
	measure_6();
	measure_7();
}

void play(unsigned int note, unsigned int duration_ticks)
{
	// plays a note for a set duration
	ms_duration = duration_ticks * ms_per_tick;		// compute duration
	current_note = note;							// set current note
	sound_enabled = OUTMOD_4;						// play note
	ms_elapsed = 0;									// reset counter
	while (ms_elapsed < ms_duration - SILENCE_MS);	// play until SILENCE
	sound_enabled = 0;								// stop note
	while (ms_elapsed < ms_duration);				// pause until duration
}

void rest(unsigned int duration_ticks)
{
	// pauses in silence for a set duration
	unsigned int ms_duration = 0;
	ms_duration = duration_ticks * ms_per_tick;		// compute duration
	sound_enabled = 0;								// stop note
	ms_elapsed = 0;									// reset counter
	while(ms_elapsed < ms_duration);				// pause
}

// INTERRUPT HANDLERS
void interrupt delay_handler()
{
	// measures the elapsed time in milliseconds
	switch (TAIV) {
		case 2:
			TACCR1 += 1000;	// == 1ms; 1clocktick = 1/1MHz = 1microsecond
			ms_elapsed++;
			break;
	}
}
ISR_VECTOR(delay_handler, ".int08")

void interrupt note_handler()
{
	// toggles the output to play a note at a given frequency
	TACCR0 += current_note;			// advance 'alarm' time
	TACCTL0 = CCIE + sound_enabled;	// sets the speakers on or off
}
ISR_VECTOR(note_handler, ".int09")

void interrupt button_handler()
{
	if (P1IFG & BUTTON) {
		P1IFG &= ~BUTTON;	// reset the interrupt flag
		play ^= 1;
	}
}
ISR_VECTOR(button_handler, ".int02")
