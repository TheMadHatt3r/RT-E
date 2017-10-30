/*
	Collin Matthews - 2014
	library to contain time functions using the Teensy 3.1 RTC
*/
#ifndef RTE_Time_h
#define RTE_Time_h

#include "Arduino.h"
#include <Time.h>  

class RTE_Time
{
  public:
	time_t t;
	volatile int time_adj_reg_hh;
	volatile int time_adj_reg_mm;
	volatile boolean time_update_needed;
  
    RTE_Time();
	void incHr(void);
    void incMin(void);
	void performRTCUpdate(void);
	String getTimeForFileName(void);
	String getDateForLCD(void);
	String getTimeForLCD(void);
	String leadingDigits(String str);
	void manualSetRTC(int s, int m, int h, int dd, int mm, int yy);
	void updateSystemClock(void); //verify it works.
	
  private:
	time_t getTeensy3Time(void);
	

};

#endif















