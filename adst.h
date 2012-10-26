/*
 * Auto DST support for VFD Modular Clock
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

#ifndef ADST_H_
#define ADST_H_

typedef struct  {
  uint8_t Month; 
  uint8_t DOTW; 
  uint8_t Week; 
  uint8_t Hour;
} DST_Rule;
typedef struct  { 
	DST_Rule Start;
	DST_Rule End;
  uint8_t Offset; 
} DST_Rules;

char* dst_setting(uint8_t dst);
uint8_t dotw(uint8_t year, uint8_t month, uint8_t day);
uint8_t getDSToffset(tmElements_t* te, DST_Rules* rules);
//void save_dstrules(uint8_t mods[]);
//long DSTseconds(uint8_t month, uint8_t doftw, uint8_t n, uint8_t hour);
//long YearSeconds(uint8_t yr, uint8_t mo, uint8_t da, uint8_t h, uint8_t m, uint8_t s);

#endif