/*
	Collin Matthews - 2014
	library to contain time functions using the Teensy 3.1 RTC
*/
#include "Arduino.h"
#include "RTE_Time.h"
#include <Time.h>  

RTE_Time::RTE_Time()
{
  time_adj_reg_hh=0;
  time_adj_reg_mm=0;
  time_update_needed=false;
  //Work around since I can't get the auto sync to work.
  t = Teensy3Clock.get();
  setTime(t);
  // set the Time library to use Teensy 3.0's RTC to keep time
  //setSyncProvider(this.Teensy3Clock.get);
  //update variables every 30 seconds from the RTC
  //setSyncInterval(30);
}

void RTE_Time::incHr()
{
	time_adj_reg_hh++;
	time_update_needed = true;
}

void RTE_Time::incMin()
{
	time_adj_reg_mm++;
	time_update_needed = true;
}

//updates RTC with new time input from interrupt buttons or a serial request?
void RTE_Time::performRTCUpdate()
{
	if(time_update_needed){
		t = now();
		//Check for role-over event.
		if(time_adj_reg_hh + hour() > 23)
			time_adj_reg_hh = time_adj_reg_hh-24;
		if(time_adj_reg_mm + minute() > 59)
			time_adj_reg_mm = time_adj_reg_mm-60;	
		//update time.
		setTime(time_adj_reg_hh + hour(), time_adj_reg_mm + minute(), second(), day(), month(), year());
		t = now();
		Teensy3Clock.set(t);
		time_adj_reg_hh = 0;
		time_adj_reg_mm = 0;
		time_update_needed = false;
	}
}

//generates the FAT 8.3 name format date... yymmddxx.xxx
String RTE_Time::getTimeForFileName()
{
	String name = "";
	name = leadingDigits(String(year()-2000)) + leadingDigits(month()) + leadingDigits(day());
	return name;
}

//Gets the formatted date for display on an LCD
String RTE_Time::getDateForLCD()
{
	String str = "";
	str = String(year()) + "-" + leadingDigits(month()) + "-" + leadingDigits(day());
	return str;
}

//Gets the formatted time for display on an LCD
String RTE_Time::getTimeForLCD()
{
	String str = "";
	str = String(hour()) + ":" + String(minute()) + ":" + String(second());
	return str;
}

// Formats string with leading 0 if missing one.
String RTE_Time::leadingDigits(String str){
  if(str.length() < 2)
	return "0" + str;
  else
	return str;
}

//call to manually set the systems RTC. USe this to set initial clock for Teensy RT-E board.
void RTE_Time::manualSetRTC(int s, int m, int h, int dd, int mm, int yy){
	setTime(h,m,s,dd,mm,yy);
	t = now();
	Teensy3Clock.set(t);
}


void RTE_Time::updateSystemClock(){
  t = Teensy3Clock.get();
  setTime(t);
}



