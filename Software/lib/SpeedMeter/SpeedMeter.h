/*
	Collin Matthews - 2014
	library to measure fuel flow and temperature.
	Just call IncPulseCnt in the edge interrupt, feed it a temperature every now and then,
	and then call FlowUpdate.
	Lib version 1.1
	1.0 - did not self contain temperature
*/
#ifndef SpeedMeter_h
#define SpeedMeter_h

#include "Arduino.h"

class SpeedMeter
{
	#define POINTS_AVERAGED 5   //how many 2 second points are averaged together for instant flow measurment. recommend ~(4-7)
	#define MS_TO_HR		3600
    #define MPH_TO_KMPH		1.60934
  public:
    SpeedMeter(long pulses_per_mile);
	void incPulseCnt(void);
    void speedUpdate(void);
	void clearSessionCnt(void);
	unsigned long     pulse_count_session;
	unsigned long     pulse_count_cnt;
	float    speed_mph;
	float    speed_kmh;
	
  private:
	long ppm;
	float rate_avg[POINTS_AVERAGED];
	unsigned long time_last_call;
	
};

#endif