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

#include "piezo.h"
#include <util/delay.h>
#include <avr/interrupt.h>

extern uint8_t g_volume;
volatile uint16_t beep_counter = 0;

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
void beep(uint16_t freq, uint8_t dur) {
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

