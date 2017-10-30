/*
	Collin Matthews - 2014
	library to measure fuel flow and temperature.
	Just call IncPulseCnt in the edge interrupt, feed it a temperature every now and then,
	and then call FlowUpdate.
	Lib version 1.1
	1.0 - did not self contain temperature
*/
#ifndef FlowLog_h
#define FlowLog_h

#include "Arduino.h"
#include "SD.h"

class FlowLog
{

  public:
	FlowLog();
	void initLogging(String filename, String headers);
	void endLogging();
	boolean writeToLog(String data_line);
	String getFreeFilename(String date_str);
	boolean logging_en;

  private:
	File myFile;
	String current_log_file;
	String logtext;
	boolean log_lcd_toggle_bit;
	char filename_chr[12];
	
};

#endif