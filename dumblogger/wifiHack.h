/******************************************************************************
 *  wifiHack.h
 *  Project: datalogger
 *
 *  Created by Donald D Davis on 7/6/12.
 *
 *  The MIT License
 *
 *  Copyright 2012 Suspect Devices. 
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use, copy,
 * modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *****************************************************************************/
#ifndef __WIFI_HACK_H__
#define __WIFI_HACK_H__ 1

#include "wirish.h"
#include "iwdg.h"
#include <ctype.h>
//#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <time.h>

//defines for toConsole.
#include "Monitor.h"

// should be a common utils.h
// #define  Watchdog_Reset() iwdg_feed()

#define  Watchdog_Reset()
#ifndef uint8_t 
#define uint8_t uint8
#define uint16_t uint16 
#define uint32_t uint32 
#endif
#ifndef GRN_LED
#define GRN_LED 19
#define ORN_LED 18
#define RED_LED 14
#endif
#define LED_ON  LOW
#define LED_OFF HIGH

#define MAX_PATCHUBE_KEY_LENGHT 80
#define MAX_PATCHUBE_FEED_LENGHT 10
#define MAX_NETWORK_PASSWORD_LENGTH 40
#define MAX_NETWORK_SSID_LENGTH 64
#define MAX_IP_LENGTH 16

extern char networkPassword[];
extern char networkSSID[];
extern char pachubeKey[];
extern char pachubeFeed[];
extern char pachubeIP[];

//extern DataStream console;
extern time_t epoch;

//extern DataStream radio;

extern bool hasWifiModule;


uint8_t NSCaction(uint8_t source);
//int networkScan(void);
int isErrorOrOK(char *);
time_t radioDate(void);
bool setRadioDate(char *);
int toPachube(int, char *);
void parseDate(char *, char *);
int pachubeDate(void);
int networkJoin(char *);
int networkSetPassword(char *);
int networkStatus();
void setupRadioModule(void);
#endif