/*
 * This file is part of the stm32-template project.
 *
 * Copyright (C) 2020 Johannes Huebner <dev@johanneshuebner.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/* This file contains all parameters used in your project
 * See main.cpp on how to access them.
 * If a parameters unit is of format "0=Choice, 1=AnotherChoice" etc.
 * It will be displayed as a dropdown in the web interface
 * If it is a spot value, the decimal is translated to the name, i.e. 0 becomes "Choice"
 * If the enum values are powers of two, they will be displayed as flags, example
 * "0=None, 1=Flag1, 2=Flag2, 4=Flag3, 8=Flag4" and the value is 5.
 * It means that Flag1 and Flag3 are active -> Display "Flag1 | Flag3"
 *
 * Every parameter/value has a unique ID that must never change. This is used when loading parameters
 * from flash, so even across firmware versions saved parameters in flash can always be mapped
 * back to our list here. If a new value is added, it will receive its default value
 * because it will not be found in flash.
 * The unique ID is also used in the CAN module, to be able to recover the CAN map
 * no matter which firmware version saved it to flash.
 * Make sure to keep track of your ids and avoid duplicates. Also don't re-assign
 * IDs from deleted parameters because you will end up loading some random value
 * into your new parameter!
 * IDs are 16 bit, so 65535 is the maximum
 */

//Define a version string of your firmware here
#define VER 1.00AK

/* Entries must be ordered as follows:
   1. Saveable parameters (id != 0)
   2. Temporary parameters (id = 0)
   3. Display values
 */
//Next param id (increase when adding new parameter!): 15
//Next value Id: 2013
/*              category     name         unit       min     max     default id */
#define PARAM_LIST \
	PARAM_ENTRY(CAT_IO,    	   	Mode,       	OPMODES,    0,      1,      0,   1) \
	PARAM_ENTRY(CAT_IO,        	Pot1,    		"",     	0,      255,    64, 2) \
	PARAM_ENTRY(CAT_IO,        	Pot2,    		"",     	0,      255,    64, 3) \
	PARAM_ENTRY(CAT_IO,        	Pot3,    		"",     	0,      255,    64, 4) \
	PARAM_ENTRY(CAT_IO,        	Pot4,    		"",     	0,      255,    64, 5) \
	PARAM_ENTRY(CAT_IO,        	TimeOut,    	"",     	0,      100,    60,  6) \
	PARAM_ENTRY(CAT_IO,        	CanCtrl,    	OFFON,     	0,      1,      0,   7) \
	PARAM_ENTRY(CAT_IO,         NodeId,     	"",     	1,      63,     10,   8) \
	PARAM_ENTRY(CAT_TIM3,       Tim3_Frequency, FREQ,       1,      5,  	3,   9) \
	PARAM_ENTRY(CAT_TIM3,       PWM3_CH3,     	OFFON,     	0,      1,      0,   10)\
    PARAM_ENTRY(CAT_TIM3,       Tim3_3_DC,   	"",        	1,     	100, 	50,  11)\
	PARAM_ENTRY(CAT_TIM4,       Tim4_Frequency, FREQ,       1,      5,  	3,   12)\
	PARAM_ENTRY(CAT_TIM4,       PWM4_CH1,     	OFFON,     	0,      1,      0,   13)\
    PARAM_ENTRY(CAT_TIM4,       Tim4_1_DC,   	"",        	1,     	100, 	50,  14)\
	VALUE_ENTRY(MODE,        	OPMODES,	2000 )\
	VALUE_ENTRY(RunTime,      	"Sec",		2014 )\
	VALUE_ENTRY(IGN,			OFFON, 		2001 )\
	VALUE_ENTRY(CHARGE,			OFFON, 		2002 )\
	VALUE_ENTRY(BMS1,			OFFON, 		2003 )\
	VALUE_ENTRY(BMS2, 	   	   	OFFON,   	2004 )\
	VALUE_ENTRY(DigiPot1, 	   	"%",   		2005 )\
	VALUE_ENTRY(DigiPot2, 	   	"%",   		2006 )\
	VALUE_ENTRY(DigiPot3, 	   	"%",   		2007 )\
	VALUE_ENTRY(DigiPot4, 	   	"%",   		2008 )\
	VALUE_ENTRY(PWM3CH3,       	OFFON,		2009 )\
	VALUE_ENTRY(PWM3CH3_DC,    	"Count",	2010 )\
	VALUE_ENTRY(PWM4CH1,       	OFFON,		2011 )\
	VALUE_ENTRY(PWM4CH1_DC,    	"Count",	2012 )\
	VALUE_ENTRY(version,       	VERSTR,		2013 )\
    VALUE_ENTRY(CPU_LOAD,       "%", 		2500 )


/***** Enum String definitions *****/
#define OPMODES      "0=OFF, 1=RUN"
#define FREQ         "1=100Hz, 2=500Hz, 3=1kHz, 4=10kHz, 5=100kHz"
#define OFFON        "0=OFF, 1=ON"
#define CAT_IO   	 "Digital I/O Control"
#define CAT_TIM3     "TIM 3 Control"
#define CAT_TIM4     "TIM 4 Control"

#define VERSTR STRINGIFY(4=VER)

/***** enums ******/

enum _modes
{
    MOD_OFF = 0,
    MOD_RUN,
    MOD_LAST
};

//Generated enum-string for possible errors
extern const char* errorListString;
