/*
	Collin Matthews - 2014
	library to measure fuel flow and temperature.
	Just call IncPulseCnt in the edge interrupt, feed it a temperature every now and then,
	and then call FlowUpdate.
	Lib version 1.1
	1.0 - did not self contain temperature
*/
#ifndef FlowMeter_h
#define FlowMeter_h

#include "Arduino.h"

class FlowMeter
{

	#define FUEL_CONT_GAL_HOUR 3600000UL //3600000UL //  (Gal*hour)*1000ms => 1ga*3600s/hr*1000ms
	#define FUEL_CONT_LITER_HOUR 136274780L //13627476UL //  (Liter/gal*hour)*1000ms => 3.78541L/ga*3600s/hr*1000ms
	#define FUEL_COMPENSATION_BASE_TEMP 15.55F  //Temperature in *C; 60=15.55*C, 68=20*C
	#define FUEL_COMPENSATION_PER_DEG 0.0007831F  // THIS is PER *C 
	#define FLOW_POINTS_AVERAGED 5   //how many 2 second points are averaged together for instant flow measurment. recommend ~(4-7)
  public:
    FlowMeter(byte units, long ppg);
	void incPulseCnt(void);
    void flowUpdate(void);
	void clearSessionCnt(void);

	unsigned long     pulse_count_total;
	unsigned long     pulse_count_session;
	unsigned long     pulse_count_cnt;
	float    fuel_consumption_rate;
	float    fuel_consumption_rate_comp;
	float    fuel_temperture_c;
	
  private:
    byte units;
	long ppg;
	float fuel_rate_avg[FLOW_POINTS_AVERAGED];
	unsigned long time_last_call;
	
};

#endif