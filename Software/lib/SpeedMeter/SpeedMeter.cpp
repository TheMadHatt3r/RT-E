/*
	Collin Matthews - 2014
	library to measure fuel flow and temperature.
	Just call IncPulseCnt in the edge interrupt, feed it a temperature every now and then,
	and then call FlowUpdate.
	Lib version 1.1
	1.0 - did not self contain temperature
*/
#include "Arduino.h"
#include "SpeedMeter.h"

SpeedMeter::SpeedMeter(long pulses_per_mile)
{
	ppm=pulses_per_mile;
	pulse_count_session=0;
	pulse_count_cnt=0;
	speed_mph=0;
	speed_kmh=0;
}

void SpeedMeter::incPulseCnt()
{
	pulse_count_session++;
	pulse_count_cnt++;
}

void SpeedMeter::clearSessionCnt()
{
	pulse_count_session=0;
	pulse_count_cnt=0;
}

void SpeedMeter::speedUpdate()
{
  unsigned int time_delta;
  unsigned long mills_holder;
  
  //Approximately the interval since last update, this makes it more accurate
  mills_holder = millis();
  time_delta = mills_holder - time_last_call;  //time since last pulse
  time_last_call = mills_holder;
  
  //Shift all readings back 1 spot
  for (byte i=(POINTS_AVERAGED-1);i>0; i--)
  {
    rate_avg[i]=rate_avg[i-1];
  }

  cli(); //ATOMIC operation
  //Calculate real moving rate!
  //best case for floating point precision is ~6 digits, double is = float in Arduino
  //always do math in mph, later convert to kmh
  rate_avg[0] = (float(pulse_count_cnt)/ppm)*MS_TO_HR;
  pulse_count_cnt=0;
  sei();//END ATOMIC operation
  
  //avg
  speed_mph=0;
  for (byte i=0;i<POINTS_AVERAGED; i++)
  {
    speed_mph = speed_mph + rate_avg[i];
  }
  speed_mph=float(speed_mph)/POINTS_AVERAGED;
  
  //also convert for Km/hr
  speed_kmh = speed_mph*MPH_TO_KMPH;
}
