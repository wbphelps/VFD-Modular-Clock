/*
 * GPS support for VFD Modular Clock
 * (C) 2012-2013 William B Phelps
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

#include <avr/interrupt.h>
#include <string.h>
#include <util/delay.h>
#include <stdlib.h>
#include "globals.h"
#include "gps.h"
#include "display.h"
#include "piezo.h"
#include "rtc.h"
#include "Time.h"
//#include "xstrtok.h"

// globals from main.c
extern enum shield_t shield;

//volatile uint8_t gpsEnabled = 0;
#define gpsTimeoutLimit 5  // 5 seconds until we display the "no gps" message
time_t gpsTimeout;  // how long since we received valid GPS data?
time_t tLast = 0;  // for checking GPS messages

void GPSread(void) 
{
  char c = 0;
  if (UCSR0A & _BV(RXC0)) {
		c=UDR0;  // get a byte from the port
		if (c == '$') {  // start of a new message?
//			gpsNextBuffer[gpsBufferPtr] = 0;
			gpsBufferPtr = 0;  // reset buffer pointer to beginning
			gpsNextBuffer[gpsBufferPtr] = c;  // add char to current buffer
		}
		else if (c == '\n') {  // newline marks end of sentence
			gpsNextBuffer[gpsBufferPtr] = 0;  // terminate string
			if (gpsNextBuffer == gpsBuffer1) {  // switch buffers
				gpsNextBuffer = gpsBuffer2;
				gpsLastBuffer = gpsBuffer1;
			} else {
				gpsNextBuffer = gpsBuffer1;
				gpsLastBuffer = gpsBuffer2;
			}
			gpsBufferPtr = 0;
			gpsDataReady_ = true;  // signal data ready
		}
		else
			gpsNextBuffer[gpsBufferPtr] = c;  // add char to current buffer
		if (gpsBufferPtr < (GPSBUFFERSIZE-1))  // if there is still room in buffer
			gpsBufferPtr++;  // increment index
	}
//	return c;
}

uint8_t gpsDataReady(void) {
	return gpsDataReady_;
}

char *gpsNMEA(void) {
  gpsDataReady_ = false;
  return (char *)gpsLastBuffer;
}

uint32_t parsedecimal(char *str, uint8_t len) {
  uint32_t d = 0;
//  while (str[0] != 0) {
	for (uint8_t i=0; i<len; i++) {
   if ((str[i] > '9') || (str[0] < '0'))
     return d;  // no more digits
	 d = (d*10) + (str[i] - '0');
//   str++;
  }
  return d;
}
const char hex[17] = "0123456789ABCDEF";
uint8_t atoh(char x) {
  return (strchr(hex, x) - hex);
}
uint32_t hex2i(char *str, uint8_t len) {
  uint32_t d = 0;
	for (uint8_t i=0; i<len; i++) {
	 d = (d*10) + (strchr(hex, str[i]) - hex);
	}
	return d;
}

// find next token in GPS string - find next comma, then point to following char
char * ntok ( char *ptr ) {
	ptr = strchr(ptr, ',');  // Find the next comma
	if (ptr == NULL) return NULL;
	ptr++;  // point at next char after comma
	return ptr;
}

//  225446       Time of fix 22:54:46 UTC
//  A            Navigation receiver warning A = OK, V = warning
//  4916.45,N    Latitude 49 deg. 16.45 min North
//  12311.12,W   Longitude 123 deg. 11.12 min West
//  000.5        Speed over ground, Knots
//  054.7        Course Made Good, True
//  191194       Date of fix  19 November 1994
//  020.3,E      Magnetic variation 20.3 deg East
//  *68          mandatory checksum

//$GPRMC,225446.000,A,4916.45,N,12311.12,W,000.5,054.7,191194,020.3,E,*68\r\n
// 0         1         2         3         4         5         6         7
// 0123456789012345678901234567890123456789012345678901234567890123456789012
//    0     1       2    3    4     5    6   7     8      9     10  11 12
// From John L
//$GPRMC,080826.000,A,3351.3136,S,15109.5939,E,0.38,206.05,010812,,,A*7E
// VK-162
//$GPRMC,200618.00,A,3726.31733,N,12207.44383,W,0.373,,300413,,,A*66
// Wikipedia
//$GPRMC,092750.000,A,5321.6802,N,00630.3372,W,0.02,31.66,280511,,,A*43
// There may be a comma after Magnetic variation direction... or not?

void parseGPSdata(char *gpsBuffer) {
	time_t tNow, tDelta;
	tmElements_t tm;
	uint8_t gpsCheck1, gpsCheck2;  // checksums
//	char gpsTime[10];  // time including fraction hhmmss.fff
	char gpsFixStat;  // fix status
//	char gpsLat[7];  // ddmm.ff  (with decimal point)
//	char gpsLatH;  // hemisphere 
//	char gpsLong[8];  // dddmm.ff  (with decimal point)
//	char gpsLongH;  // hemisphere 
//	char gpsSpeed[5];  // speed over ground
//	char gpsCourse[5];  // Course
//	char gpsDate[6];  // Date
//	char gpsMagV[5];  // Magnetic variation 
//	char gpsMagD;  // Mag var E/W
//	char gpsCKS[2];  // Checksum without asterisk
	char *ptr;
  uint32_t tmp;
	if ( strncmp( gpsBuffer, "$GPRMC,", 7 ) == 0 ) {  
		//beep(1000, 1);
		//Calculate checksum from the received data
		ptr = &gpsBuffer[1];  // start at the "G"
		gpsCheck1 = 0;  // init collector
		 /* Loop through entire string, XORing each character to the next */
		while (*ptr != '*') // count all the bytes up to the asterisk
		{
			gpsCheck1 ^= *ptr;
			ptr++;
			if (ptr>(gpsBuffer+GPSBUFFERSIZE)) goto ParseError;  // extra sanity check, can't hurt...
		}
		// now get the checksum from the string itself, which is in hex
    gpsCheck2 = atoh(*(ptr+1)) * 16 + atoh(*(ptr+2));
		if (gpsCheck1 == gpsCheck2) {  // if checksums match, process the data
			//beep(1000, 1);
			ptr = &gpsBuffer[1];  // start at beginning of buffer
			ptr = ntok(ptr);  // Find the time string
			if (ptr == NULL) goto ParseError;
			char *p2 = strchr(ptr, ',');  // find comma after Time
			if (p2 == NULL) goto ParseError;
//			char ll = p2 - ptr - 1;
//			if ((ll < 6) || (ll > 10)) goto ParseError2;  // check time length
			if (p2 < (ptr+6)) goto ParseError;  // Time must be at least 6 chars
//			strncpy(gpsTime, ptr, 10);  // copy time string hhmmss
			tmp = parsedecimal(ptr,6);   // parse integer portion
			tm.Hour = tmp / 10000;
			tm.Minute = (tmp / 100) % 100;
			tm.Second = tmp % 100;
			ptr = ntok(ptr);  // Find the next token - Status
			if (ptr == NULL) goto ParseError;
			gpsFixStat = ptr[0];  // fix status
			if (gpsFixStat == 'A') {  // if data valid, parse time & date
				gpsTimeout = 0;  // reset gps timeout counter
				for (uint8_t n=0; n<7; n++) { // skip to 7th next token - date
					ptr = ntok(ptr);  // Find the next token 
					if (ptr == NULL) goto ParseError;
				}
				p2 = strchr(ptr, ',');  // find comma after Date
				if (p2 == NULL) goto ParseError;
//				ll = p2 - ptr - 1;
				if (p2 != (ptr+6)) goto ParseError;  // check date length
				tmp = parsedecimal(ptr, 6); 
				tm.Day = tmp / 10000;
				tm.Month = (tmp / 100) % 100;
				tm.Year = tmp % 100;
//				ptr = ntok(ptr);  // Find the next token - Mag variation
//				if (ptr == NULL) goto ParseError;
//				ptr = ntok(ptr);  // Find the next token - Mag var direction
//				if (ptr == NULL) goto ParseError;
				ptr = strchr(ptr, '*');  // Find Checksum
				if (ptr == NULL) goto ParseError;
//				strncpy(gpsCKS, ptr, 2);  // save checksum chars
				
				tm.Year = y2kYearToTm(tm.Year);  // convert yy year to (yyyy-1970) (add 30)
				tNow = makeTime(&tm);  // convert to time_t
//				g_gps_updating = false;
				
//				if ((tGPSupdate>0) && (abs(tNow-tGPSupdate)>SECS_PER_DAY))  goto TimeError;  // GPS time jumped more than 1 day
				// only accept GPS data if 2 messages in sequence are reasonably close in time
				// this helps catch corrupt message strings
				// the GPS emits a GPRMC message once a second
				if ( (tLast>0) && (abs(tNow - tLast)>30) )  // if time jumps by more than a few seconds, 
				{
					tLast = tNow;  // save new time
					goto TimeError;  // it's probably an error
				}
				else {
					tLast = tNow;
					tDelta = tNow - tGPSupdate;
#ifdef FEATURE_FLASH_GPS_RECEIVED
					flash_gps_rcvd();  // show GPS RMC message received ???
#endif
					if (((tm.Second<5) && (tDelta>10)) || (tDelta>=60)) {  // update RTC once/minute or if it's been 60 seconds
						//beep(1000, 1);  // debugging
//						g_gps_updating = true;
						tGPSupdate = tNow;  // remember time of this update
						flash_gps_update();  // show GPS is updating
						tNow = tNow + (long)(globals.TZ_hour + globals.DST_offset) * SECS_PER_HOUR;  // add time zone hour offset & DST offset
						if (globals.TZ_hour < 0)  // add or subtract time zone minute offset
							tNow = tNow - (long)globals.TZ_minute * SECS_PER_HOUR;
						else
							tNow = tNow + (long)globals.TZ_minute * SECS_PER_HOUR;
						rtc_set_time_t(tNow);  // set RTC from adjusted GPS time & date
//						if ((shield != SHIELD_IV18) && (shield != SHIELD_IV17))
//							flash_display();  // flash display to show GPS update 28oct12/wbp - shorter blink
					}
//					else
//						g_gps_updating = false;
				}
			} // if fix status is A
		} // if checksums match
		else  // checksums do not match
			g_gps_cks_errors++;  // increment error count
		return;
//ParseError3:
//		beep(1100,200);  // error beep
//		goto ParseError;
//ParseError2:
//		beep(880,200);  // error beep
ParseError:
		g_gps_parse_errors++;  // increment error count
		beep(1100,100);  // error beep
		goto Error2a;
TimeError:
		g_gps_time_errors++;  // increment error count
		beep(2200,100);  // error signal - I'm leaving this in for now /wm
Error2a:
//		flash_display();  // flash display to show GPS error
		strcpy(gpsBuffer, "");  // wipe GPS buffer
	}  // if "$GPRMC"
}

void uart_init(uint16_t BRR) {
  /* setup the main UART */
  UBRR0 = BRR;               // set baudrate counter
  UCSR0B = _BV(RXEN0) | _BV(TXEN0);
  UCSR0C = _BV(USBS0) | (3<<UCSZ00);
  DDRD |= _BV(PD1);
  DDRD &= ~_BV(PD0);
}

void gps_init(uint8_t gps) {
	switch (gps) {
		case(0):  // no GPS
			break;
		case(48):
			uart_init(BRRL_4800);
			break;
		case(96):
			uart_init(BRRL_9600);
			break;
	}
	tGPSupdate = 0;  // reset GPS last update time
	gpsDataReady_ = false;
  gpsBufferPtr = 0;
  gpsNextBuffer = gpsBuffer1;
  gpsLastBuffer = gpsBuffer2;
}
