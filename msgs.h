/*
 * Messages for VFD Modular Clock
 * (C) 2013 William B Phelps
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

#ifndef MSGS_H_
#define MSGS_H_

//#define messages 4
//uint8_t msg_Dates[messages][2] = { {3,1}, {1,1}, {1,4}, {12,25} };
//char* msg_Texts[messages] = {"Happy Birthday John", "Happy New Year", "Happy Perihelion", "Merry Christmas"};

//#define messages 3
//uint8_t msg_Dates[messages][2] = { {1,14}, {1,1}, {12,25} };
//char* msg_Texts[messages] = {"Happy Birthday Simone", "Happy New Year", "Merry Christmas"};

#define messages 4 // number of messages
uint8_t msg_Dates[messages][2] = { {3,14}, {1,1}, {1,4}, {12,25} };
char* msg_Texts[messages] = {"Happy Birthday William", "Happy New Year",  "Happy Perihelion", "Merry Christmas"};
uint8_t msg_Count = messages;

#endif
