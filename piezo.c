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

#ifdef FEATURE_BIGBEN
short freqs[] = {130.81, 138.59, 146.83, 155.56, 164.81, 174.61, 185.00, 196.00, 207.65, 220.00, 233.08, 246.94};
const uint8_t Cn=0;
const uint8_t Cs=1;
const uint8_t Df=1;
const uint8_t Dn=2;
const uint8_t Ds=3;
const uint8_t Ef=3;
const uint8_t En=4;
const uint8_t Ff=4;
const uint8_t Fn=5;
const uint8_t Fs=6;
const uint8_t Gf=6;
const uint8_t Gn=7;
const uint8_t Gs=8;
const uint8_t Af=8;
const uint8_t An=9;
const uint8_t As=10;
const uint8_t Bf=10;
const uint8_t Bn=11;
const uint16_t tempo = 16;

void note(uint8_t n, uint8_t octave, uint16_t timing) {
	double m = 1;
	for (uint8_t i = 0; i<(octave-3); i++) {
		m *= 2;
	}
	double f = freqs[n] * m;
	uint16_t t = timing * tempo;
	beep(f, t);
	_delay_ms(tempo/2);
}

uint16_t bT = 16;
uint8_t bO = 5;
void ben1(uint8_t lnd) {
	note(Gs,bO,bT);
	note(Fs,bO,bT);
	note(En,bO,bT);
	note(Bn,bO-1,bT*2);
	_delay_ms(250);
}
void ben2(uint8_t lnd) {
	note(En,bO,bT);
	note(Gs,bO,bT);
	note(Fs,bO,bT);
	note(Bn,bO-1,bT*lnd);
	_delay_ms(250);
}
void ben3(uint8_t lnd) {
	note(En,bO,bT);
	note(Fs,bO,bT);
	note(Gs,bO,bT);
	note(En,bO,bT*lnd);
	_delay_ms(250);
}
void ben4(uint8_t lnd) {
	note(Gs,bO,bT);
	note(En,bO,bT);
	note(Fs,bO,bT);
	note(Bn,bO-1,bT*lnd);
	_delay_ms(250);
}
void ben5(uint8_t lnd) {
	note(Bn,bO-1,bT);
	note(Fs,bO,bT);
	note(Gs,bO,bT);
	note(En,bO,bT*lnd);
	_delay_ms(250);
}
#endif

// pizeo code from: https://github.com/adafruit/Ice-Tube-Clock
void piezo_init(void) {
	PEZ_DDR |= _BV(PEZ1) | _BV(PEZ2);
	PEZ_PORT &= ~_BV(PEZ1) & ~_BV(PEZ2);
	TCCR1A = _BV(COM1A1) | _BV(COM1B1) | _BV(COM1B0) | _BV(WGM11);  // set OC1B on match, clear COM1A1, fast pwm
	TCCR1B = _BV(WGM13) | _BV(WGM12);
}

// F_CPU = 8000000 (8 MHz), TIMER1 clock is 1000000 (1 MHz)
// ICR1 (Top) = 1000000/freq - interrupt at freq to pulse spkr
// TIMER1 interrupt rate = freq = 1000/freq ms
// at low frequencies, time resolution is lower
// example: at 100 hz, beep timer resolution is 10 ms
void beep(uint16_t freq, uint16_t dur) {
  // set the PWM output to match the desired frequency
  uint16_t top = F_CPU/8/freq;  // set Top
	uint16_t cm = (top>>8) * (g_volume+2);  // set duty cycle based on volume
	ICR1 = top;
	OCR1A = cm;
  OCR1B = top - OCR1A;

  // beep for the requested time
	beep_counter = (long)dur * freq  / 1000;  // set delay counter
	TCNT1 = 0; // Initialize counter
  TCCR1B |= _BV(CS11); // connect clock to turn speaker on
	TIMSK1 |= (1<<TOIE1); // Enable Timer1 Overflow Interrupt
	_delay_ms(dur);

}

void beepEnd(void) {
	TIMSK1 &= ~(1<<TOIE1);  // disable Timer1 
  TCCR1B &= ~_BV(CS11); // disconnect clock source to turn it off
  // turn speaker off
  PEZ_PORT &= ~_BV(PEZ1) & ~_BV(PEZ2);
}

ISR(TIMER1_OVF_vect)
{
	if (beep_counter > 0)
		beep_counter--;
	else {
		beepEnd();
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
  _delay_ms(5);
  PEZ_PORT |= _BV(PEZ2);
  PEZ_PORT &= ~_BV(PEZ1);
  _delay_ms(5);
  // turn them both off
  PEZ_PORT &= ~_BV(PEZ1) & ~_BV(PEZ2);
	// restore volume setting - 12oct12/wbp
  TCCR1A = _BV(COM1B1) | _BV(COM1B0) | _BV(WGM11);
  if (g_volume) {  // 12oct12/wbp
    TCCR1A |= _BV(COM1A1);
  } 
  TCCR1B = _BV(WGM13) | _BV(WGM12);
}

void alarm(void)
{
	beep(500, 100);
}

