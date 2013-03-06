/*
 * VFD Modular Clock
 * (C) 2011 Akafugu Corporation
 *
 * This program is free software; you can redistribute it and/or modify it under the
 * terms of the GNU General Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any later
 * version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
 * PARTICULAR PURPOSE.  See the GNU General Public License for more details.
 *
 */

#include <avr/io.h>
#include "globals.h"
#include "piezo.h"
#include <util/delay.h>
#include <avr/interrupt.h>

//extern uint8_t g_volume;
volatile uint16_t beep_counter = 0;
volatile uint16_t pause_counter = 0;
volatile uint8_t alarming = 0;
volatile uint8_t beeping = 0;

void piezo_init(void) {
	PEZ_DDR |= _BV(PEZ1) | _BV(PEZ2);
	PEZ_PORT &= ~_BV(PEZ1) & ~_BV(PEZ2);
//	TCCR1A = _BV(COM1A1) | _BV(COM1B1) | _BV(COM1B0) | _BV(WGM11);  // set OC1B on match, clear COM1A1, fast pwm
	TCCR1A = _BV(WGM11);  // no pwm
	TCCR1B = _BV(WGM13) | _BV(WGM12);
}

// F_CPU = 8000000 (8 MHz), TIMER1 clock is 1000000 (1 MHz)
// ICR1 (Top) = 1000000/freq - interrupt at freq to pulse spkr
// TIMER1 interrupt rate = freq = 1000/freq ms
// at low frequencies, time resolution is lower
// example: at 100 hz, beep timer resolution is 10 ms
void setFreq(uint16_t freq) {
  // set the PWM output to match the desired frequency
  uint16_t top = F_CPU/8/freq;  // set Top
	uint16_t cm = (top>>8) * (g_volume+2);  // set duty cycle based on volume
	ICR1 = top;
	OCR1A = cm;
  OCR1B = top - OCR1A;
}

void tone(uint16_t freq, uint16_t dur) {
	setFreq(freq);
  // beep for the requested time
	beep_counter = (long)dur * freq  / 1000;  // set delay counter
	beeping = 1;  // set flag for interrupt routine
	TCNT1 = 0; // Initialize counter
	TCCR1A = _BV(COM1A1) | _BV(COM1B1) | _BV(COM1B0) | _BV(WGM11);  // set OC1B on match, clear COM1A1, fast pwm
  TCCR1B |= _BV(CS11); // connect clock to start timer
	TIMSK1 |= (1<<TOIE1); // Enable Timer1 Overflow Interrupt
}

void beep(uint16_t freq, uint16_t dur) {
	tone(freq, dur);
	_delay_ms(dur);
}

void pause(uint16_t dur) {
	setFreq(1000);  // 1 ms per timer tick
  // pause for the requested time
	pause_counter = dur;  // set delay counter
	TCNT1 = 0; // Initialize counter
	TCCR1A = _BV(WGM11);  // no pwm
  TCCR1B |= _BV(CS11); // connect clock to start timer
	TIMSK1 |= (1<<TOIE1); // Enable Timer1 Overflow Interrupt
}

#ifdef FEATURE_REVEILLE
short freqs[] = {130.81, 138.59, 146.83, 155.56, 164.81, 174.61, 185.00, 196.00, 207.65, 220.00, 233.08, 246.94};
#define Cn 0
#define Cs 1
#define Df 1
#define Dn 2
#define Ds 3
#define Ef 3
#define En 4
#define Ff 4
#define Fn 5
#define Fs 6
#define Gf 6
#define Gn 7
#define Gs 8
#define Af 8
#define An 9
#define As 10
#define Bf 10
#define Bn 11
const uint16_t tempo = 16;

void note(uint8_t n, uint8_t octave, uint16_t timing) {
	if (octave>0) {
		double m = 1;
		for (uint8_t i = 0; i<(octave-3); i++) {
			m *= 2;
		}
		double f = freqs[n] * m;
		uint16_t t = timing * tempo;
		tone(f, t);
		pause_counter = tempo*4;  // pause after tone
	}
	else
		pause(tempo*timing);
}

volatile uint16_t i_reveille = 0;
#define rO 6
#define rT 8
const uint8_t reveille[] = {
	Gn,rO-1,rT, Cn,rO,rT, En,rO,rT/2, Cn,rO,rT/2, Gn,rO-1,rT, En,rO,rT, 
	Cn,rO,rT, En,rO,rT/2, Cn,rO,rT/2, Gn,rO-1,rT, En,rO,rT,
	Cn,rO,rT, En,rO,rT/2, Cn,rO,rT/2, Gn,rO-1,rT, Cn,rO,rT, En,rO,rT*2, Cn,rO,rT,
	0,0,rT,
	Gn,rO-1,rT, Cn,rO,rT, En,rO,rT/2, Cn,rO,rT/2, Gn,rO-1,rT,	En,rO,rT,
	Cn,rO,rT, En,rO,rT/2, Cn,rO,rT/2, Gn,rO-1,rT,	En,rO,rT,
	Cn,rO,rT, En,rO,rT/2, Cn,rO,rT/2, Gn,rO-1,rT, Gn,rO-1,rT, Cn,rO,rT*3,
	0,0,rT*2,
	En,rO,rT, En,rO,rT, En,rO,rT, En,rO,rT, En,rO,rT, Gn,rO,rT*2, En,rO,rT,
	0,0,rT,
	Cn,rO,rT, En,rO,rT, Cn,rO,rT, En,rO,rT, Cn,rO,rT, En,rO,rT*2, Cn,rO,rT,
	0,0,rT,
	En,rO,rT, En,rO,rT, En,rO,rT, En,rO,rT, En,rO,rT, Gn,rO,rT*2, En,rO,rT,
	0,0,rT,
	Cn,rO,rT, En,rO,rT, Cn,rO,rT, Gn,rO-1,rT, Gn,rO-1,rT, Cn,rO,rT*3,
	0,0,rT*4,
};

void play_reveille() {  // play first bar of reveille
	alarm(1);
	_delay_ms(600);
	alarm(0);
}
#endif

void nextNote(void) {
	_delay_ms(tempo);
	i_reveille+=3;
	if (i_reveille>=sizeof(reveille))
		i_reveille = 0;  // wrap
	note(reveille[i_reveille],reveille[i_reveille+1],reveille[i_reveille+2]);
}

void beepEnd(void) {
//	TIMSK1 &= ~(1<<TOIE1);  // disable Timer1 
//  TCCR1B &= ~_BV(CS11); // disconnect clock source to turn it off
	TCCR1A = _BV(WGM11);  // stop pwm
  // turn speaker off
  PEZ_PORT &= ~_BV(PEZ1) & ~_BV(PEZ2);
	beeping = 0;
	if (pause_counter > 0)
		setFreq(1000);  // change interrupt frequency for timing
}

void pauseEnd(void) {
	TIMSK1 &= ~(1<<TOIE1);  // disable Timer1 
  TCCR1B &= ~_BV(CS11); // disconnect clock source to turn it off
}

ISR(TIMER1_OVF_vect)
{
	if (beeping) {
		if (beep_counter > 0)
			beep_counter--;
		else 
			beepEnd();  // stop the beep now
	}
	else { // not beeping, so pausing
		if (pause_counter > 0)
			pause_counter--;
		else {
			pauseEnd();  // done with note 
			if (alarming)
				nextNote();  // play the next note
		}
	}
}

// This makes the speaker tick, it doesnt use PWM
// instead it just flicks the piezo
void tick(void) {
  TCCR1A = 0;
  TCCR1B = 0;
  // Send a pulse thru both pins, alternating
  PEZ_PORT |= _BV(PEZ1);
  PEZ_PORT &= ~_BV(PEZ2);
  _delay_ms(4);
  PEZ_PORT |= _BV(PEZ2);
  PEZ_PORT &= ~_BV(PEZ1);
  _delay_ms(4);
  // turn them both off
  PEZ_PORT &= ~_BV(PEZ1) & ~_BV(PEZ2);
	TCCR1A = _BV(WGM11);  // restore TCCR1
  TCCR1B = _BV(WGM13) | _BV(WGM12);
}

static uint8_t s_volume;
void alarm(uint8_t ena)
{
//	beep(500, 100);
	if (ena) {
		if (!alarming) {
			s_volume = g_volume;  // save current volume setting
			g_volume = g_volume<<2;  // increase volume for higher frequency notes
			alarming = 1;
			i_reveille = 0;  // start with first note
			note(reveille[i_reveille],reveille[i_reveille+1],reveille[i_reveille+2]);
		}
	}
	else {
		beepEnd();
		pauseEnd();
		if (alarming)  // did we save volume?
			g_volume = s_volume;
		alarming = 0;
	}
}

