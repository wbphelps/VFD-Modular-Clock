/*
 * Globals for VFD Modular Clock
 * (C) 2011-2012 Akafugu Corporation
 * (C) 2012 William B Phelps
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
//#define FEATURE_AUTO_DIM  // moved to Makefile

#include <util/delay.h>
#include <avr/eeprom.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include "globals.h"

#define EE_CHECK 42 // change this value if you change EE addresses
#define EE_globals 0 // eeprom address

// Settings saved to eeprom
struct __globals globals = {
	EE_CHECK,
	true, // clock_24h
	false, // show temp
	false, // show humid
	false, // show press
	true, // show dots
	8, // brightness
	0, // volume
	13, 1, 1, // year, month, day
	ALARM_NORMAL, // alarm type
	false, // snooze enabled
#ifdef FEATURE_FLW
	false, // flw enabled
#endif
#ifdef FEATURE_WmGPS
	false, // gps enabled
	0, 0, // TZ hour, minute
#endif
#if defined FEATURE_WmGPS || defined FEATURE_AUTO_DST
	0, 0, // DST mode, offset
#endif
#ifdef FEATURE_AUTO_DST  // DST rules
#ifdef DST_NSW
	{ 10,1,1,2, 4,1,1,3, 1 },
#else
	{ 3,1,2,2, 11,1,1,2, 1 },
#endif
#endif
#ifdef FEATURE_AUTO_DATE
	0, // date format
	false, // auto date
#endif
#ifdef FEATURE_AUTO_DIM
	false, // auto dim
	7, 8, // auto dim1 hour, level
	19, 5, // auto dim2 hour, level
	22, 2, // auto dim3 hour, level
#endif
	EE_CHECK,
};

void save_globals()
{
	for (unsigned int p=0; p<sizeof(globals); p++) {
		uint8_t b1 = eeprom_read_byte((uint8_t *)EE_globals + p);
		if (b1 != *((char *) &globals + p))
			eeprom_write_byte((uint8_t *)EE_globals + p, *((char*)&globals + p));
	}
}

void globals_init(void) 
{
	uint8_t ee_check1 = eeprom_read_byte((uint8_t *)EE_globals + (&globals.EEcheck1-&globals.EEcheck1));
	uint8_t ee_check2 = eeprom_read_byte((uint8_t *)EE_globals + (&globals.EEcheck2-&globals.EEcheck1));
	if ((ee_check1!=EE_CHECK) || (ee_check2!=EE_CHECK)) { // has EE been initialized?
		for (unsigned int p=0; p<sizeof(globals); p++) { // copy globals structure to EE memory
			eeprom_write_byte((uint8_t *)EE_globals + p, *((char*)&globals + p));
		}
	}
	else { // read globals from EE
		for (unsigned int p=0; p<sizeof(globals); p++) // read gloabls from EE
			*((char*)&globals + p) = eeprom_read_byte((uint8_t *)EE_globals + p);
	}
}

// uint8_t EEMEM b_dummy = 0;  // dummy item to test for bug
// uint8_t EEMEM b_24h_clock = 1;
// uint8_t EEMEM b_show_temp = 0;
// uint8_t EEMEM b_show_dots = 1;
// uint8_t EEMEM b_brightness = 8;
// uint8_t EEMEM b_volume = 10;
// #ifdef FEATURE_FLW
// uint8_t EEMEM b_flw_enabled = 0;
// #endif
// #ifdef FEATURE_WmGPS
// uint8_t EEMEM b_gps_enabled = 0;  // 0, 48, or 96 - default no gps
// uint8_t EEMEM b_TZ_hour = -8 + 12;
// uint8_t EEMEM b_TZ_minute = 0;
// #endif
// #if defined FEATURE_WmGPS || defined FEATURE_AUTO_DST
// uint8_t EEMEM b_DST_mode = 0;  // 0: off, 1: on, 2: Auto
// uint8_t EEMEM b_DST_offset = 0;
// #endif
// #ifdef FEATURE_AUTO_DATE
// uint8_t EEMEM b_Region = 0;  // default European date format Y/M/D
// uint8_t EEMEM b_AutoDate = 0;
// #endif
// #ifdef FEATURE_AUTO_DIM
// uint8_t EEMEM b_AutoDim = 0;
// uint8_t EEMEM b_AutoDimHour = 22;
// uint8_t EEMEM b_AutoDimLevel = 2;
// uint8_t EEMEM b_AutoBrtHour = 7;
// uint8_t EEMEM b_AutoBrtLevel = 8;
// #endif
// #ifdef FEATURE_AUTO_DST
// //DST_Rules dst_rules = {{3,1,2,2},{11,1,1,2},1};   // initial values from US DST rules as of 2011
// //DST_Rules dst_rules = {{10,1,1,2},{4,1,1,2},1};   // DST Rules for parts of OZ including NSW (for JG)
// //#define DST_NSW
// #ifdef DST_NSW
// uint8_t EEMEM b_DST_Rule0 = 10;  // DST start month
// uint8_t EEMEM b_DST_Rule1 = 1;  // DST start dotw
// uint8_t EEMEM b_DST_Rule2 = 1;  // DST start week
// uint8_t EEMEM b_DST_Rule3 = 2;  // DST start hour
// uint8_t EEMEM b_DST_Rule4 = 4; // DST end month
// uint8_t EEMEM b_DST_Rule5 = 1;  // DST end dotw
// uint8_t EEMEM b_DST_Rule6 = 1;  // DST end week
// uint8_t EEMEM b_DST_Rule7 = 2;  // DST end hour
// uint8_t EEMEM b_DST_Rule8 = 1;  // DST offset
// #else
// uint8_t EEMEM b_DST_Rule0 = 3;  // DST start month
// uint8_t EEMEM b_DST_Rule1 = 1;  // DST start dotw
// uint8_t EEMEM b_DST_Rule2 = 2;  // DST start week
// uint8_t EEMEM b_DST_Rule3 = 2;  // DST start hour
// uint8_t EEMEM b_DST_Rule4 = 11; // DST end month
// uint8_t EEMEM b_DST_Rule5 = 1;  // DST end dotw
// uint8_t EEMEM b_DST_Rule6 = 1;  // DST end week
// uint8_t EEMEM b_DST_Rule7 = 2;  // DST end hour
// uint8_t EEMEM b_DST_Rule8 = 1;  // DST offset
// #endif
// #endif
