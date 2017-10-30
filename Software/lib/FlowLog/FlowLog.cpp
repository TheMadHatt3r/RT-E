/*
	Collin Matthews - 2014
	
*/
#include "Arduino.h"
#include "FlowLog.h"
#include "SD.h"



FlowLog::FlowLog()
{
	current_log_file = "";
	logtext = "";
	log_lcd_toggle_bit = false;
	logging_en = false;
}




void FlowLog::initLogging(String filename, String headers){
	logging_en = true;
	current_log_file = filename;
	current_log_file.toCharArray(filename_chr,12);
	myFile = SD.open(filename_chr, FILE_WRITE);
	myFile.println(headers);
	myFile.close();
}



void FlowLog::endLogging(){
	logging_en = false;
	current_log_file = "";
}

boolean FlowLog::writeToLog(String data_line){
	if(logging_en & current_log_file != ""){
		myFile = SD.open(filename_chr, FILE_WRITE);
		myFile.println(data_line);
		myFile.close();
		return true;
	}
	else
		return false;
}

String FlowLog::getFreeFilename(String date_str){
	String proposed_file = "";

	for (byte cnt = 0; cnt <100; cnt++){
		if (cnt < 10)
			proposed_file = date_str + "0" + String(cnt) + ".CSV";
		else
			proposed_file = date_str + String(cnt) + ".CSV";
		//break loop if open file name found.
		proposed_file.toCharArray(filename_chr,12);
		if(!SD.exists(filename_chr))
			break;
		//Flag if no open files...
		if(cnt == 99)
			proposed_file = "XXX";
	}
	
	return proposed_file;
}


