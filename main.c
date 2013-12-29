/*
 * VFD Modular Clock
 * (C) 2011-2013 Akafugu Corporation
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

/* Updates by William B Phelps
*todo:
 * ?
 
 * 09dec13 separate include for messages 
 * 05may13 add new data check in parseGPSdata()
 * 06apr13 fix auto dst southern hemisphere bug
 * 10mar13 add millis()
 * 07mar13 new timer driven scroll
 * 06mar13 snooze feature
 * 06mar13 stop alarm if switched off while sounding
 * 05mar13 add vars for birthday message
 * 04mar13 fix crash caused by display mpx timer int
 * 22feb13 10 step volume
 * 02jan13 generalize scrolling, add holiday messages
 * 30nov12 fix rule0 limits (again)
 * 26nov12 put menu_items in PROGMEM!
 * 25nov12 add alarm & time to menu
 * 25nov12 fix time/alarm set repeat interval
 *  put menu_values in PROGMEM
 *  remove FEATURE_AUTO_MENU
 * 21nov12 reduce RAM usage, in general and for avr-gcc 4.7.2
 * 19nov12 redo main loop timing now that GPS read is interrupt driven
 * 18nov12 move FEATURE_ADIM to Makefile, cleanup FEATURE ifdefs
 * 17nov12 right-align string displays for 4 digit display
 *  tested wtih all 3 displays
 * 16nov12 fix auto dim for iv-17 display
 * 16nov12 - reduced ram 
 *  sub sub menu (for DST Rules)
 *  add tick for button push in menu
 * 15nov12 - fix bug when setting dst auto
 * 14nov12 table driven Menu
 * 12nov12 add Auto dim/brt feature
 * 11nov12 interrupt driven GPS read
 *  add local FEATURE_GPS_DEBUG to control if gps debug counters are in menu
 * 10nov12 add gps error counters to menu
 * 09nov12 rewrite GPS parse
 * 08nov12 auto menu feature
 * 07nov12 check GPS time vs. previous, add GPS error counter
 * 06nov12 rtc: separate alarm time, adst: add DSTinit function
 * 05nov12 fix DST calc bug
 *  speed up alarm check in rtc (not done)
 * 03nov12 fix for ADST falling back
 *  change region to "DMY/MDY/YMD"
 * 30oct12 menu changes - single menu block instead of 2
 * 29oct12 "gps_updating" flag, use segment on IV18 to show updates
 *  shift time 1 pos for 12 hour display
 * 25oct12 implement Auto DST
 * 24oct12 fix date display for larger displays
 * 22oct12 add DATE to menu, add FEATURE_SET_DATE and FEATURE_AUTO_DATE
 *  added to menu: YEAR, MONTH, DAY
 * 12oct12 fix blank display when setting time (blink)
 *  return to AMPM after AutoDate display
 * 05oct12 clean up GPS mods for commit
 *  add get/set time in time_t to RTC
 *  slight blink when RTC updated from GPS to show signal reception
 * 26sap12
 *  fix EUR date display
 *  add GPS 48, 96 (support 9600 baud GPS) 
 * 25sep12 improve date scrolling
 *  add Time.h/Time.c, use tmElements_t structure instead of tm
 *  add '/' to font-16seg.c
 * 24sep12 - add time zone & DST menu items
 *  show current value without change on 1st press of b1
 *  scroll the date for smaller displays
 * 21sep12 - GPS support
 * 02sep12 - post to Github
 * 28aug12 - clean up display multiplex code
 * 27aug12 - 10 step brightness for IV-17
 * 26aug12 - display "off" after "alarm" if alarm switch off
 * 25aug12 - change Menu timeout to ~2 seconds
 *  show Alarm time when Alarm switch is set to enable the alarm
 *  get time from RTC every 200 ms instead of every 150 ms
 *  blank hour leading zero if not 24 hr display mode
 *  minor typos & cleanup
 */

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
//#include <avr/eeprom.h>
#include <avr/wdt.h>     // Watchdog timer to repair lockups

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "Time.h"
#include "globals.h"
#include "menu.h"
#include "display.h"
#include "button.h"
#include "twi.h"
#include "rtc.h"
#ifdef FEATURE_FLW
#include "flw.h"
#endif
#include "piezo.h"

#ifdef FEATURE_AUTO_DST
#include "adst.h"
#endif
#ifdef FEATURE_WmGPS
#include "gps.h"
#endif
#ifdef FEATURE_MESSAGES
#include "msgs.h"
#endif

// Second indicator LED (optional second indicator on shield)
#define LED_BIT PD7
#define LED_DDR DDRD
#define LED_PORT PORTD
#define LED_HIGH LED_PORT |= _BV(LED_BIT)
#define LED_HIGH LED_PORT |= _BV(LED_BIT)
#define LED_LOW  LED_PORT &= ~(_BV(LED_BIT))

// Piezo
#define PIEZO_PORT PORTB
#define PIEZO_DDR  DDRB
#define PIEZO_LOW_BIT  PB1
#define PIEZO_HIGH_BIT PB2
#define PIEZO_LOW_OFF  PIEZO_PORT &= ~(_BV(PIEZO_LOW_BIT))
#define PIEZO_HIGH_OFF PIEZO_PORT &= ~(_BV(PIEZO_HIGH_BIT))
#define PIEZO_LOW_ON  PIEZO_PORT |= _BV(PIEZO_LOW_BIT)
#define PIEZO_HIGH_ON PIEZO_PORT |= _BV(PIEZO_HIGH_BIT)

// Cached settings
#ifdef FEATURE_AUTO_DST
//DST_Rules dst_rules = {{10,1,1,2},{4,1,1,2},1};   // DST Rules for parts of OZ including NSW (for JG)
//DST_Rules dst_rules = {{3,1,2,2},{11,1,1,2},1};   // initial values from US DST rules as of 2011
//uint8_t dst_rules[] = {3,1,2,2,11,1,1,2,1};   // initial values from US DST rules as of 2011
// DST Rules: Start(month, dotw, week, hour), End(month, dotw, week, hour), Offset
// DOTW is Day of the Week, 1=Sunday, 7=Saturday
// N is which occurrence of DOTW
// Current US Rules:  March, Sunday, 2nd, 2am, November, Sunday, 1st, 2 am, 1 hour
#endif

#define MENU_TIMEOUT 20  // 2.0 seconds

//#ifdef FEATURE_AUTO_DATE
//uint16_t g_autodisp = 50;  // how long to display date 5.0 seconds
//#endif
uint8_t g_autotime = 55;  // controls when to display date and when to display time in FLW mode

uint8_t g_alarming = false; // alarm is going off
uint16_t snooze_count = 0; // alarm snooze counter
uint16_t alarm_timer = 0; // how long has alarm been beeping?
uint16_t alarm_count, alarm_cycle, beep_count, beep_cycle;
uint8_t g_snooze_time = 7; // snooze for 7 minutes
uint8_t g_alarm_switch;
uint16_t g_show_special_cnt = 0;  // display something special ("time", "alarm", etc)
//time_t g_tNow = 0;  // current local date and time as time_t, with DST offset
tmElements_t* tm_; // current local date and time as TimeElements (pointer)
//uint8_t alarm_hour = 0, alarm_min = 0, alarm_sec = 0;

extern enum shield_t shield;

void initialize(void)
{

	PIEZO_DDR |= _BV(PIEZO_LOW_BIT);
	PIEZO_DDR |= _BV(PIEZO_HIGH_BIT);
	
	PIEZO_LOW_OFF;
	PIEZO_HIGH_OFF;

	// Set buttons as inputs
	BUTTON_DDR &= ~(_BV(BUTTON1_BIT));
	BUTTON_DDR &= ~(_BV(BUTTON2_BIT));
	SWITCH_DDR  &= ~(_BV(SWITCH_BIT));
	
	// Enable pullups for buttons
	BUTTON_PORT |= _BV(BUTTON1_BIT);
	BUTTON_PORT |= _BV(BUTTON2_BIT);
	SWITCH_PORT |= _BV(SWITCH_BIT);

	LED_DDR  |= _BV(LED_BIT); // indicator led

	for (int i = 0; i < 5; i++) {
		LED_HIGH;
		_delay_ms(100);
		LED_LOW;
		_delay_ms(100);
	}
	
	sei();
	twi_init_master();
	
	rtc_init();
	rtc_set_ds1307();

	//rtc_set_time_s(16, 59, 50);
	//rtc_set_alarm_s(17,0,0);

	piezo_init();
	globals_init();
	beep(440, 75);
	_delay_ms(75);
	display_init(g_brightness);
	
	g_alarm_switch = get_alarm_switch();

	tm_ = rtc_get_time();

#ifdef FEATURE_FLW
	g_has_eeprom = has_eeprom();
//	if (!g_has_eeprom)  beep(440,200);  // debug
	if (!g_has_eeprom)
		g_flw_enabled = false;
	if (tm_ && g_has_eeprom)
		seed_random(tm_->Hour * 10000 + tm_->Minute + 100 + tm_->Second);
#endif

	beep(880, 75);
	_delay_ms(75);
	menu_init();  // must come after display, flw init
	rtc_get_alarm_s(&alarm_hour, &alarm_min, &alarm_sec);

	// set up interrupt for alarm switch
	PCICR |= (1 << PCIE2);
	PCMSK2 |= (1 << PCINT18);
	
#ifdef FEATURE_WmGPS
  // setup uart for GPS
	gps_init(g_gps_enabled);
#endif
#ifdef FEATURE_AUTO_DST
	if (g_DST_mode)
		DSTinit(tm_, g_DST_Rules);  // re-compute DST start, end	
#endif
}

// display modes
typedef enum {
	MODE_NORMAL = 0, // normal mode: show time/seconds
	MODE_AMPM, // shows time AM/PM
#ifdef FEATURE_FLW
	MODE_FLW,  // shows four letter words
#endif
#ifdef FEATURE_AUTO_DATE
	MODE_DATE, // shows date
#endif
	MODE_LAST,  // end of display modes for right button pushes
	MODE_ALARM_TEXT,  // show "alarm" (wm)
	MODE_ALARM_TIME,  // show alarm time (wm)
} display_mode_t;
struct BUTTON_STATE buttons;

display_mode_t clock_mode = MODE_NORMAL;
display_mode_t save_mode = MODE_NORMAL;  // for restoring mode after autodate display
//uint8_t menu_update = false;

// Alarm switch changed interrupt
ISR( PCINT2_vect )
{
	if ( (SWITCH_PIN & _BV(SWITCH_BIT)) == 0) {
		g_alarm_switch = false;
		if (g_alarming) {
			g_alarming = false;  // cancel alarm
			snooze_count = 0;  // and snooze
			set_blink(false);
		}
	}
	else {
		g_alarm_switch = true;
	}
	g_show_special_cnt = 10;  // show alarm text for 1 second
	if (get_digits() == 8)
		clock_mode = MODE_ALARM_TIME;
	else
		clock_mode = MODE_ALARM_TEXT;
}

#ifdef FEATURE_AUTO_DATE
void display_date(void)
{
// display the date (or other message) once a minute just before the minute changes
// the timing is tuned such that the clock shows the time again at 59 1/2 seconds
#ifdef FEATURE_MESSAGES
	clock_mode = MODE_DATE;  // displaying date now
	show_string(" ");  // blank normal time display
	set_scroll(300);  // display date at 3 cps
  uint8_t sd = true; // show date if no message
  for (uint8_t i=0; i<msg_Count; i++) {
    if ((tm_->Month == msg_Dates[i][0]) && (tm_->Day == msg_Dates[i][1])) {
			show_scroll(msg_Texts[i]);  // show message
			set_scroll(250);  // 4 cps
			sd = false;
    }
  }
  if (sd)
#endif
	scroll_date(tm_, g_Region);  // show date from last rtc_get_time() call
}
#endif

void display_time(void)  // (wm)  runs approx every 100 ms
{
	static uint16_t counter = 0;
	
	if (clock_mode == MODE_ALARM_TEXT) {
		show_alarm_text();
	}
	else if (clock_mode == MODE_ALARM_TIME) {
		if (g_alarm_switch) {
			rtc_get_alarm_s(&alarm_hour, &alarm_min, &alarm_sec);
			show_alarm_time(alarm_hour, alarm_min, 0);
		}
		else {
			show_alarm_off();
		}
	}
// alternate temp and time every 2.5 seconds
	else if (g_show_temp && rtc_is_ds3231() && counter > 250) {
		int8_t t;
		uint8_t f;
		ds3231_get_temp_int(&t, &f);
		show_temp(t, f);
	}
	else {
		tm_ = rtc_get_time();
		if (tm_ == NULL) return;
#ifdef FEATURE_SET_DATE
		g_dateyear = tm_->Year;  // save year for Menu
		g_datemonth = tm_->Month;  // save month for Menu
		g_dateday = tm_->Day;  // save day for Menu
#endif
#ifdef FEATURE_AUTO_DST
		if ((tm_->Hour == 0) && (tm_->Minute == 0) && (tm_->Second == 0)) {  // MIDNIGHT!
			g_DST_updated = false;
			if (g_DST_mode)
				DSTinit(tm_, g_DST_Rules);  // re-compute DST start, end
		}
		if (g_DST_mode) {
			if (tm_->Second%10 == 0)  // check DST Offset every 10 seconds (60?)
				setDSToffset(tm_, g_DST_mode);
		}
#endif
#ifdef FEATURE_AUTO_DATE
		if (clock_mode == MODE_DATE) {
			if (!scrolling())  // Still scrolling date or message?
				clock_mode = save_mode;  // no, restore previous mode
		}
		else if (g_AutoDate && (tm_->Second == g_autotime) ) { 
			save_mode = clock_mode;  // save current mode
			display_date();  // show date or other message
		}
#endif		
#ifdef FEATURE_FLW
		if (clock_mode == MODE_FLW) {
			if ((tm_->Second >= g_autotime - 3) && (tm_->Second < g_autotime))
				show_time(tm_, g_24h_clock, 0); // show time briefly each minute
			else
				show_flw(tm_); // otherwise show FLW
		}
		else
#endif
		show_time(tm_, g_24h_clock, clock_mode);
	}

	counter++;
	if (counter == 500) counter = 0;

	if (g_show_special_cnt>0) {
		g_show_special_cnt--;
		if (g_show_special_cnt == 0) {
			switch (clock_mode) {
				case MODE_ALARM_TEXT:
					clock_mode = MODE_ALARM_TIME;
					g_show_special_cnt = 10;
					break;
				case MODE_ALARM_TIME:
					clock_mode = MODE_NORMAL;
					break;
				default:
					clock_mode = MODE_NORMAL;
			}
		}
	}
	
}

void start_alarm(void)
{
	g_alarming = true;
	snooze_count = 0;
	alarm_cycle = 200;  // 20 second initial cycle
	alarm_count = 0;  // reset cycle count
	beep_cycle = 2;  // start with single beep
	alarm_timer = 0;  // restart start alarm timer
	set_blink(true);
}

void main(void) __attribute__ ((noreturn));

void main(void)
{
	initialize();

	/*
	// test: write alphabet
	while (1) {
		for (int i = 'A'; i <= 'Z'+1; i++) {
			set_char_at(i, 0);
			set_char_at(i+1, 1);
			set_char_at(i+2, 2);
			set_char_at(i+3, 3);

			if (get_digits() == 6) {
				set_char_at(i+4, 4);
				set_char_at(i+5, 5);
			}

			_delay_ms(250);
		}
	}
	*/
	
//	uint8_t hour = 0, min = 0, sec = 0;

	// Counters used when setting time
	uint16_t button_released_timer = 0;
	uint16_t button_speed = 2;

	switch (shield) {
		case(SHIELD_IV6):
			show_string("IV-6");
			break;
		case(SHIELD_IV17):
			show_string("IV17");
			break;
		case(SHIELD_IV18):
			show_string("IV18");
			break;
		case(SHIELD_IV22):
			show_string("IV22");
			break;
		default:
			break;
	}

	beep(440, 75);

	_delay_ms(500);
	//set_string("--------");

#ifdef FEATURE_MESSAGES
// uncomment these to display a message when the clock is plugged in
//	show_scroll("Hello World");
//	show_scroll(bdMsg);  // show birthday message
	show_scroll(msg_Texts[0]);  // show birthday message
	_delay_ms(500);
#endif

unsigned long t1, t2;
	
	while (1) {  // << ===================== MAIN LOOP ===================== >>
		t1 = millis();
		get_button_state(&buttons);
		if (scrolling()) {
			if (buttons.b1_keyup || buttons.b2_keyup) {  // either button down stops scrolling
				scroll_stop();
				buttons.b2_keyup = 0;  // don't change time display
			}
		}
		if (snooze_count>0)
			snooze_count--;
		if (g_alarming) {
			alarm_timer ++;
			if (alarm_timer > 30*60*10) {  // alarm has been sounding for 30 minutes, turn it off
				g_alarming = false;
				snooze_count = 0;
				set_blink(false);
			}
		}
		// When alarming: any button press snoozes alarm
		if (g_alarming && (snooze_count==0)) {
			display_time();  // read and display time
			// fixed: if keydown is detected here, wait for keyup and clear state
			// this prevents going into the menu when disabling the alarm 
			if (buttons.b1_keydown || buttons.b1_keyup || buttons.b2_keydown || buttons.b2_keyup) {
				buttons.b1_keyup = 0; // clear state
				buttons.b2_keyup = 0; // clear state
				start_alarm();  // restart alarm sequence
				snooze_count = g_snooze_time*60*10;  // start snooze timer
				show_snooze();
				_delay_ms(500);
				while (buttons.b1_keydown || buttons.b2_keydown) {  // wait for button to be released
					_delay_ms(100);
					get_button_state(&buttons);
				}
			}
			else {
				alarm_count++;
				if (alarm_count > alarm_cycle) {  // once every alarm_cycle
					beep_count = alarm_count = 0;  // restart cycle 
					if (alarm_cycle>20)  // if more than 2 seconds
						alarm_cycle = alarm_cycle - 20;  // shorten delay by 2 seconds
					if (beep_cycle<20)
						beep_cycle += 2;  // add another beep
				}
				beep_count++;
				if (beep_count <= beep_cycle) {  // how many beeps this cycle?
					if (beep_count%2)  // odd = beep
						alarm();
				}
			}
		}
		// If both buttons are held:
		//  * If the ALARM BUTTON SWITCH is on the LEFT, go into set time mode
		//  * If the ALARM BUTTON SWITCH is on the RIGHT, go into set alarm mode
		else if (menu_state == STATE_CLOCK && buttons.both_held) {
			g_alarming = false;  // setting time or alarm, cancel alarm
			snooze_count = 0;  // and snooze
			set_blink(false);  // stop blinking
			if (g_alarm_switch) {
				menu_state = STATE_SET_ALARM;
				show_set_alarm();
				rtc_get_alarm_s(&alarm_hour, &alarm_min, &alarm_sec);
				time_to_set = alarm_hour*60 + alarm_min;
			}
			else {
				menu_state = STATE_SET_CLOCK;
				show_set_time();		
				rtc_get_time_s(&hour, &min, &sec);
				time_to_set = hour*60 + min;
			}

			set_blink(true);
			
			// wait until both buttons are released
			while (1) {
				_delay_ms(50);
				get_button_state(&buttons);
				if (buttons.none_held)
					break;
			}
		}
		// Set time or alarm
		else if (menu_state == STATE_SET_CLOCK || menu_state == STATE_SET_ALARM) {
			// Check if we should exit STATE_SET_CLOCK or STATE_SET_ALARM
			if (buttons.none_held) {
				set_blink(true);
				button_released_timer++;
				button_speed = 1;
			}
			else {
				set_blink(false);
				button_released_timer = 0;
				button_speed++;
			}

			// exit mode after no button has been touched for a while
			if (button_released_timer >= MENU_TIMEOUT) {
				set_blink(false);
				button_released_timer = 0;
				button_speed = 1;
				
				// fixme: should be different in 12h mode
				if (menu_state == STATE_SET_CLOCK) {
					rtc_set_time_s(time_to_set / 60, time_to_set % 60, 0);
#if defined FEATURE_AUTO_DST
					g_DST_updated = false;  // allow automatic DST adjustment again
#endif
				}
				else {
					alarm_hour = time_to_set / 60;
					alarm_min  = time_to_set % 60;
					alarm_sec  = 0;
					rtc_set_alarm_s(alarm_hour, alarm_min, alarm_sec);
				}

				menu_state = STATE_CLOCK;
			}
			
			// Increase / Decrease time counter
			if (buttons.b1_repeat) time_to_set+=(button_speed/10);
			if (buttons.b1_keyup)  time_to_set++;
			if (buttons.b2_repeat) time_to_set-=(button_speed/10);
			if (buttons.b2_keyup)  time_to_set--;

			if (time_to_set  >= 1440) time_to_set = 0;
			if (time_to_set  < 0) time_to_set = 1439;

			show_time_setting(time_to_set / 60, time_to_set % 60, 0);
		}
		else if (menu_state == STATE_CLOCK && buttons.b2_keyup) {  // Left button enters menu
			menu_state = STATE_MENU;
      menu(0);  // show first menu item 
			buttons.b2_keyup = 0; // clear state
		}
		// Right button toggles display mode
		else if (menu_state == STATE_CLOCK && buttons.b1_keyup) {
			clock_mode++;

#ifdef FEATURE_FLW
			if (clock_mode == MODE_FLW && !g_has_eeprom) clock_mode++;
			if (clock_mode == MODE_FLW && !g_flw_enabled) clock_mode++;
#endif

#ifdef FEATURE_AUTO_DATE
			if (clock_mode == MODE_DATE) {
				save_mode = MODE_NORMAL;  // go back to normal mode after showing date
				display_date();  // show the date or other message
			}
#endif			
			if (clock_mode == MODE_LAST) clock_mode = MODE_NORMAL;
			buttons.b1_keyup = 0; // clear state
		}
		else if (menu_state == STATE_MENU) {
			if (buttons.none_held)
				button_released_timer++;
			else
				button_released_timer = 0;
			if (button_released_timer >= MENU_TIMEOUT) {  
				button_released_timer = 0;
				menu_state = STATE_CLOCK;
			}

			if (buttons.b1_keyup) {  // right button
				menu(1);  // right button
				buttons.b1_keyup = false;
			}  // if (buttons.b1_keyup) 

			if (buttons.b2_keyup) {  // left button
				menu(2);  // left button
				buttons.b2_keyup = 0; // clear state
			}
		}
		else {

			// read RTC & update display approx every 100ms  (wm)
			display_time();  // read RTC and display time

		}
		
		// fixme: alarm should not be checked when setting time or alarm
		// fixme: alarm will be missed if time goes by Second=0 while in menu
		if (g_alarm_switch && rtc_check_alarm_cached(tm_, alarm_hour, alarm_min, alarm_sec)) {
			start_alarm();
		}

#ifdef FEATURE_AUTO_DIM			
		if ((g_AutoDim) && (tm_->Minute == 0) && (tm_->Second == 0))  {  // Auto Dim enabled?
			if (tm_->Hour == g_AutoDimHour) {
				set_brightness(g_AutoDimLevel);
			}
			else if (tm_->Hour == g_AutoBrtHour) {
				set_brightness(g_AutoBrtLevel);
			}
		}
#endif

#ifdef FEATURE_WmGPS
		if (g_gps_enabled) {
			if (gpsDataReady()) {
				parseGPSdata(gpsNMEA());  // get the GPS serial stream and possibly update the clock 
			}
		}
#endif

//		_delay_ms(60);  // tuned so loop runs 10 times a second
		t2 = millis();
		while ((t2-t1)<100)
			t2 = millis();

		}  // while (1)
}  // main()
