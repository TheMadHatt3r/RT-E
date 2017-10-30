/*
	Collin Matthews - 2014
	library to measure fuel flow and temperature.
	Just call IncPulseCnt in the edge interrupt, feed it a temperature every now and then,
	and then call FlowUpdate.
	Lib version 1.1
	1.0 - did not self contain temperature
*/
#include "Arduino.h"
#include "FlowMeter.h"

FlowMeter::FlowMeter(byte units_in, long ppg_in)
{
	ppg=ppg_in;
	units=units_in;
	pulse_count_total=0;
	pulse_count_session=0;
	pulse_count_cnt=0;
	fuel_consumption_rate=0;
	fuel_consumption_rate_comp=0;
	fuel_temperture_c=22;
}

void FlowMeter::incPulseCnt()
{
	pulse_count_total++;
	pulse_count_session++;
	pulse_count_cnt++;
}

void FlowMeter::clearSessionCnt()
{
	pulse_count_session=0;
	pulse_count_cnt=0;
}

void FlowMeter::flowUpdate()
{
  unsigned int time_delta;
  unsigned long mills_holder;
  unsigned long math_num,math_den;
  float delta_temperture_comp;
  
  //Approximately TIME_FUEL_AVERAGE, this makes it more accurate
  mills_holder = millis();
  time_delta = mills_holder - time_last_call;  //time since last pulse
  time_last_call = mills_holder;
  
  //Shift all readings back 1 spot
  for (byte i=(FLOW_POINTS_AVERAGED-1);i>0; i--)
  {
    fuel_rate_avg[i]=fuel_rate_avg[i-1];
  }

  cli(); //ATOMIC operation
  //Calculate real time fuel rate!
  //best case for floating point precision is ~6 digits, double is = float in Arduino
  if(units==1) //Gallons Per Hour
  {
    //fuel_rate_avg[0]=float(FUEL_CONT_GAL_HOUR*pulse_count_cnt)/( _GALLON*time_delta);
    math_num=FUEL_CONT_GAL_HOUR*pulse_count_cnt;
    math_den=ppg*time_delta;
    fuel_rate_avg[0] = float(math_num)/math_den;
  }
  else if(units==2) //LITER Per Hour
  {
    //fuel_rate_avg[0]=float(FUEL_CONT_LITER_HOUR*pulse_count_cnt)/(PULSES_PER_GALLON*time_delta);
    math_num = FUEL_CONT_LITER_HOUR*pulse_count_cnt;
    math_den = ppg*time_delta;
    fuel_rate_avg[0] = float(math_num)/math_den;
  }
  
  pulse_count_cnt=0;
  sei();//END ATOMIC operation
  
  //avg
  fuel_consumption_rate=0;
  for (byte i=0;i<FLOW_POINTS_AVERAGED; i++)
  {
    fuel_consumption_rate = fuel_consumption_rate + fuel_rate_avg[i];
  }
  //avg
  fuel_consumption_rate=float(fuel_consumption_rate)/FLOW_POINTS_AVERAGED;
  
  //Perform temperature compensation
  delta_temperture_comp = (FUEL_COMPENSATION_BASE_TEMP-fuel_temperture_c); // difference from base point temp (60 or 68*F) in *C
  delta_temperture_comp = delta_temperture_comp*FUEL_COMPENSATION_PER_DEG; 
  delta_temperture_comp = 1 + delta_temperture_comp;
  fuel_consumption_rate_comp = fuel_consumption_rate * delta_temperture_comp; 

}
