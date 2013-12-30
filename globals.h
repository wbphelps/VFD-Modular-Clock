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
 #ifndef GLOBALS_H_
#define GLOBALS_H_

#define FEATURE_MENU_TIME  // add alarm & time to menu
#define FEATURE_GPS_DEBUG  // enables GPS debugging counters & menu items
//#define FEATURE_MESSAGES  // holiday messages
//#define FEATURE_FLASH_GPS_RECEIVED // show when GPS message is received

#ifdef __FLASH
#define FLASH __flash
#else
#define FLASH
#endif

// date format modes
typedef enum {
  FORMAT_YMD = 0,
  FORMAT_DMY,
  FORMAT_MDY,
} date_format_t;

// alarm type
typedef enum {
  ALARM_NORMAL = 0,
  ALARM_PROGRESSIVE,
} alarm_type_t;

struct __globals
{
	uint8_t EEcheck1;
	int8_t clock_24h;
	int8_t show_temp;
	int8_t show_humid;
	int8_t show_press;
	int8_t show_dots;
	int8_t brightness;
	int8_t volume;
	int8_t dateyear;
	int8_t datemonth;
	int8_t dateday;
	int8_t alarmtype;
	int8_t snooze_enabled;
#ifdef FEATURE_FLW
	int8_t flw_enabled
#endif
#ifdef FEATURE_WmGPS 
	int8_t gps_enabled;
	int8_t TZ_hour; // offset by 12 to make positive???
	int8_t TZ_minute;
#endif
#if defined FEATURE_WmGPS || defined FEATURE_AUTO_DST
	int8_t DST_mode;  // DST off, on, auto?
	int8_t DST_offset;  // DST offset in Hours
#endif
#ifdef FEATURE_AUTO_DST  // DST rules
	int8_t DST_Rules[9];
#endif
#ifdef FEATURE_AUTO_DATE
//	date_format_t date_format;
	int8_t Region;
	int8_t AutoDate;
#endif
#ifdef FEATURE_AUTO_DIM
	int8_t AutoDim;
	int8_t AutoDimHour1;
	int8_t AutoDimLevel1;
	int8_t AutoDimHour2;
	int8_t AutoDimLevel2;
	int8_t AutoDimHour3;
	int8_t AutoDimLevel3;
#endif
	uint8_t EEcheck2;
};

int8_t alarm_hour, alarm_minute, alarm_second;
int8_t time_hour, time_minute, time_second;
uint16_t time_to_set;

#ifdef FEATURE_WmGPS 
int8_t g_gps_cks_errors;  // gps checksum error counter
int8_t g_gps_parse_errors;  // gps parse error counter
int8_t g_gps_time_errors;  // gps time error counter
#endif
#if defined FEATURE_WmGPS || defined FEATURE_AUTO_DST
uint8_t g_DST_updated;  // DST update flag = allow update only once per day
#endif
uint8_t g_has_dots; // can current shield show dot (decimal points)

void globals_init(void);
void save_globals(void);
extern struct __globals globals; // can't put this here...

#endif

