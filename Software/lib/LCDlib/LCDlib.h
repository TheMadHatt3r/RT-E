/*
  LCDlib.h - library for writing to a 20x4 Newhaven i2c LCD
  Created by Collin Matthews, 14 October 2013
  For use by Reicon Systems, all other uses prohibited without consent.
  ctmrocks2435@yahoo.com
*/

#ifndef LCDlib_h
#define LCDlib_h

#include "Arduino.h"
#include <Wire.h>

class LCDlib
{
  public:
	LCDlib();
    LCDlib(int address);
	void init();
	void bkLightOn();
	void bkLightOff();
	//clear entire screen (1.5ms 4x20)
    void clear();
	//Move to UL (1.5ms 4x20)
    void home();
	//Single Char write is 100us.
    void write_text(char x[]);
	void write_text(String x);
	void write_int(int a);
	void write_ulong(unsigned long a);
	void write_tm(int a);
	void write_float2(float b);
	void write_float1(float b);
    void goto_xy(int x, int y);
	//Set back-light brightness (1-8) 1=~off. 100us
	//Back-light Current (4x20) BL=1,10mA BL=2,30mA BL=4,70mA BL=6,95mA BL=8,140mA
    void backlight(int l);
	//Set back-light contrast (0-50)  500us
    void contrast(int c);
   private:
	int addr;
};

#endif