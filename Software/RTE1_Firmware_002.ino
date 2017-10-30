/***************************************
Collin Matthews - 23-JUL-2015
Requires following libs installed in the base arduino directory.
  -IntervalTimer, but don't include in project
  -LCDlib
  -FlowMeter
  -FlowLog
  -SpeedMeter
  -RTE_Time
  -Time
 Also Mod Wire.cpp in C:\Programs\arduino\Librarys\Wire.h for Teensy #elif F_BUS == 48000000.
   I2C0_F = 0x2F; for 50kHz, remove the 0x27 for 100kHz
   It should look like
   #elif F_BUS == 48000000
	I2C0_F = 0x2F;	// 50 kHz
 Finally, the Teensy should have a CPU clock of 48MHz, this is set via the arduio interface Tools>CPU_SPEED.
   By default it is 72 MHz
 ****************************************/


/*******************
 TODO:
 Add capacitors to board
 Turn off display on brownout to extend power
 
*******************/

//Remove depreciated string conversion warning.
#pragma GCC diagnostic ignored "-Wwrite-strings"

#include <FlowMeter.h>
//#include <IntervalTimer.h>
#include <EEPROM.h>
#include <Wire.h>
#include <LCDlib.h>
#include <SpeedMeter.h>
#include <FlowLog.h>
#include <RTE_Time.h>
#include <SD.h>
#include <Time.h>  
#include <RTE_Serial.h>


/******* CONFIG DEFINES ****************/
//These are things for you to alter with some level of safety.
#define SW_REV "002"                //the revision of software?
#define SERIAL_NUMBER "RTE001"   //Customer serial number reported at startup
#define BROWNOUT_VOLTAGE 4.5F  //Brownout Voltage 75XX-SMPS Vin min = 6.5V
#define PULSES_PER_GALLON_1 7570UL  //PPG, adjust as you need to.
#define PULSES_PER_GALLON_2 7570UL  //PPG, adjust as you need to.
#define PULSES_PER_MILE    1000UL
#define FUEL_RATE_UNITS 1        //UNITS: 1=Gal/Hr; 2=L/Hr
#define TEMP_RESISTOR_1 10000UL //This is the REAL value of the 10K resistor that forms the voltage devider with the thermistor.
#define TEMP_RESISTOR_2 10000UL //This is the REAL value of the 10K resistor that forms the voltage devider with the thermistor.
#define SYSTEM_TIMER_US_RATE 100000UL //This sets the system clock to "tick" once every .1sec, or 100,000us 
#define SD_HEADERS "Date/Time,Flow1 TC [GPH],Flow1 [GPH],Flow1 Pulses,Flow1 Temp[C],Flow2 TC [GPH],Flow2 [GPH],Flow2 Pulses,Flow2 Temp[C],Speed [MPH]"

/******* CONST DEFINES ****************/
#define TOTAL_PULSE_COUNT_BASE_ADDR_1 67  //EE Prom Address
#define TOTAL_PULSE_COUNT_BASE_ADDR_2 75  //EE Prom Address
#define TOTAL_PULSE_COUNT_LENGTH 4      //4 bytes = 32bit unsigned int (4294967296)
#define I2C_ADDR_LCD 0x28     // 0x50 >> 1 = 0x28 (real address is 0x50)

//Timer Times in 100ms intervals
#define TIME_FUEL_AVERAGE  5  //length of period fuel counts averaveraged..
#define TIME_SIG_UPDATE 2     //Update period for brownout and ambiant light sensor + misc.
#define TIME_LCD_UPDATE 10    //How often to update the LCD display.
#define TIME_LOG_UPDATE 20    //How often to take a datapoint and save it to the SD card. Recommend > 1 Second due to it writeing every time... it will burn up the flash.
#define TIME_RTC_REFRESH 300 //refresh the time variables from the RTC. this is needed because the time library is not accurate long term.

/******* PIN NAME DEFINES ****************/
#define PIN_FUEL_TEMP_1      A0
#define PIN_FUEL_TEMP_2      A1
#define PIN_LIGHT            A6
#define PIN_INPUT_VOLT       21
#define PIN_TIME_HH          A8
#define PIN_TIME_MM          A9
#define PIN_SD_INSERTED      2    //
#define PIN_FUEL_PULSE_1     5    //
#define PIN_FUEL_PULSE_2     6    //
#define PIN_SPEED            7    ////MAY CHANGE TO ANOTHER PIN!!!
#define PIN_RESET_CNT        16  //9
#define GPIO_0_LOG_BTN       9    // 16This pin is now being used to start and stop the log
#define PIN_GPIO_1           17 

/******* Global Vars **********************/
byte   update_reg;
String log_filename;
float input_voltage;
float ambiant_light;
boolean sd_card_ok;

/**** GLOBAL OBJECTS ******/
FlowMeter flowObj_1 = FlowMeter(FUEL_RATE_UNITS, PULSES_PER_GALLON_1);
FlowMeter flowObj_2 = FlowMeter(FUEL_RATE_UNITS, PULSES_PER_GALLON_2);
SpeedMeter speedObj = SpeedMeter(PULSES_PER_MILE);
LCDlib lcd = LCDlib(I2C_ADDR_LCD);
RTE_Time timeObj = RTE_Time();
RTE_Serial serialObj;
IntervalTimer myTimer;
FlowLog sd_log;

/******** FUNCION HEADERS ********/
void FlowPulseCount1(void);
void FlowPulseCount2(void);
void MyTimer1ISR(void);
float getInputVoltage(void);
String Float2String(float num, byte percision);
void CheckBrownoutAndBackup(void);

uint32_t COUNT_32; //DEBUG

/************ Void Setup *************************
* Runs once to set stuff up.
*************************************************/
void setup() {

  //PORT SET
  pinMode(PIN_FUEL_TEMP_1, INPUT); 
  pinMode(PIN_FUEL_TEMP_2, INPUT); 
  pinMode(PIN_LIGHT, INPUT);
  pinMode(PIN_INPUT_VOLT, INPUT);
  pinMode(PIN_TIME_HH, INPUT_PULLUP);
  pinMode(PIN_TIME_MM, INPUT_PULLUP);
  pinMode(PIN_SD_INSERTED, INPUT_PULLUP);  
  pinMode(PIN_FUEL_PULSE_1, INPUT);  
  pinMode(PIN_FUEL_PULSE_2, INPUT);
  pinMode(PIN_SPEED, INPUT);
  pinMode(PIN_RESET_CNT, INPUT_PULLUP);
  pinMode(GPIO_0_LOG_BTN, INPUT_PULLUP);  
  pinMode(PIN_GPIO_1,OUTPUT);
  delay(1);

  //Initialize Local and Gloabal Vars
  update_reg=0x00; //update now!
  sd_card_ok = false;
  log_filename = "";
  COUNT_32=0;
  
  //misc. setup
  analogReadResolution(12);//12 bit
  analogReadAveraging(4);
  //analogReference(EXTERNAL);
  delay(1);
  
  //checks input voltage at startup
  input_voltage = getInputVoltage();
  
  //Load pulses from EEPROM
  loadPulsesFromEEPROM();
  
  //Check for SD card
  if (SD.begin() & (digitalRead(PIN_SD_INSERTED)==LOW))
    sd_card_ok = true;

  //Start system timer interupts
  myTimer.begin(MyTimer1ISR, SYSTEM_TIMER_US_RATE);
 

  //Attacher interupt and start
  attachInterrupt(PIN_FUEL_PULSE_1, FlowPulseCount1,FALLING); //Register Falling Edge PIN
  attachInterrupt(PIN_FUEL_PULSE_2, FlowPulseCount2,FALLING); //Register Falling Edge PIN
  attachInterrupt(PIN_SPEED, SpeedPulseISR,FALLING); //Register Falling Edge PIN
  attachInterrupt(PIN_TIME_HH, TimeIncHhISR,FALLING); //Register Falling Edge PIN
  attachInterrupt(PIN_TIME_MM, TimeIncMmISR,FALLING); //Register Falling Edge PIN
  
  interrupts();
}





void loop() 
{

  /************************************
  * While loop, this is where the main loops,
  * checks flages and such...
  * If its not done in here, Its done in an ISR
  *************************************/
  //LCD Startup
  Wire.begin();
  delay(10);  
  lcd.init();
  delay(2);  
  lcd.clear();
  lcd.home();
  delay(2);  
  lcd.backlight(4);
  
  //Start up serial object
  Serial.begin(9600);//57600 is fast...
  serialObj = RTE_Serial(Serial);
  
  //Write startup message
  startupMessage();
  delay(4000);
  
    
  //************* Main while loop *******************************************
  while(1)
  {
    
    
    //********** Update with normal statistics ************
    if((update_reg&0x01)==1)
    {
      
       update_reg = update_reg&(~0x01); //clear flag
       //lcd.backlight(6);  //DEBUG!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
       lcd.clear();
       delay(1);
       lcd.home();
       //Flow 1 - line 1
       lcd.write_text("GPH1:"); //Send Flow rate w/ T comp
       lcd.write_float2(flowObj_1.fuel_consumption_rate_comp);
       lcd.write_text("  "); //Send Total Pulses
       lcd.write_ulong(flowObj_1.pulse_count_session);
       lcd.write_text("  "); //Send Total Pulses
       lcd.write_ulong(flowObj_1.pulse_count_total);
       //Flow 2 - line 2
       lcd.goto_xy(0,1);
       lcd.write_text("GPH2:"); //Send Flow rate w/ T comp
       lcd.write_float2(flowObj_2.fuel_consumption_rate_comp);
       lcd.write_text("  "); //Send Total Pulses
       lcd.write_ulong(flowObj_2.pulse_count_session);
       lcd.write_text("  "); //Send Total Pulses
       lcd.write_ulong(flowObj_2.pulse_count_total);
       
       //Break to check for brownout
       CheckBrownoutAndBackup();
       
       //Thermals - Line 3
       lcd.goto_xy(0,2);
       lcd.write_text("Temp[C]:"); //Send Fuel Temp
       lcd.write_float1(flowObj_1.fuel_temperture_c);
       lcd.write_text(" "); //Send Total Pulses
       lcd.write_float1(flowObj_2.fuel_temperture_c);
       //xxxxxx - line 4
       lcd.goto_xy(0,3);
       lcd.write_text("Vin:");
       lcd.write_float1(getInputVoltage());
       //Debug print otu speed pulses.
       lcd.goto_xy(8,3);
       lcd.write_text("MPH:");
       lcd.write_ulong(speedObj.speed_mph);
       //End debug
       if (sd_log.logging_en){
         lcd.goto_xy(16,3);
         lcd.write_text("LOG"); //Send Total Pulses
       }
       
    }
     
    //************** Update Fuel usage *********************
    if((update_reg&0x02)==2)
    {
      update_reg = update_reg&(~0x02); //clear flag
      //FlowUpdate();
      flowObj_1.fuel_temperture_c = Thermistor(analogRead(PIN_FUEL_TEMP_1), TEMP_RESISTOR_1); //Read fuel temp
      flowObj_2.fuel_temperture_c = Thermistor(analogRead(PIN_FUEL_TEMP_2), TEMP_RESISTOR_2); //Read fuel temp
      flowObj_1.flowUpdate(); //Generate new flow data
      flowObj_2.flowUpdate(); //Generate new flow data
    }
    
    //************** Log data to SD card *********************
    if((update_reg&0x04)==4)
    {
      update_reg = update_reg&(~0x04); //clear flag
      if (sd_log.logging_en){
        String temp_line="";
        temp_line += timeObj.getDateForLCD() + " " + timeObj.getTimeForLCD();
        temp_line += "," + String(flowObj_1.fuel_consumption_rate_comp) + "," + String(flowObj_1.fuel_consumption_rate);
        temp_line += "," + String(flowObj_1.pulse_count_session) + "," + Float2String(flowObj_1.fuel_temperture_c,1);      
        temp_line += "," + String(flowObj_2.fuel_consumption_rate_comp) + "," + String(flowObj_2.fuel_consumption_rate);  
        temp_line += "," + String(flowObj_2.pulse_count_session) + "," + Float2String(flowObj_2.fuel_temperture_c,1);
        temp_line += "," +Float2String(speedObj.speed_mph,1);
        //Write to log file.
        sd_log.writeToLog(temp_line);
      }

    }    
    
    //************** Misc. signals check **********************
    if((update_reg&0x08)==8)
    {
      update_reg = update_reg&(~0x08); //clear flag
      
      //Check input voltage for brownout condition
      CheckBrownoutAndBackup();
      
      //Update speed average
      speedObj.speedUpdate();

      //Check ambiant light and adjust backlight accordingly.
      ambiant_light = analogRead(PIN_LIGHT); 
      if (ambiant_light<5){
        //Do somthing
      }
      
      //If reset is pushed, clear the session count.
      if (button_pushed(PIN_RESET_CNT, LOW)){
        flowObj_1.clearSessionCnt();
        flowObj_2.clearSessionCnt();
      }
      
      //If log button is pushed, start/stop logging if SD card ok.
      if (button_pushed(GPIO_0_LOG_BTN, LOW) & sd_card_ok){
        if (sd_log.logging_en){
          sd_log.endLogging();
          delay(100);}
        else{
          log_filename = sd_log.getFreeFilename(timeObj.getTimeForFileName());
          //if log file name error
          if(log_filename=="XXX"){
            lcd.clear();
            lcd.home();
            lcd.write_text("Max Logs Reached,");
            lcd.goto_xy(0,1);
            lcd.write_text("for today (99).");
            update_reg|=0b00000001;
            delay(2000);
          }
          //if open log file found.
          else{
            sd_log.initLogging(log_filename, SD_HEADERS);
            delay(100);
          }
        }
      }

      
      //If HH or MM buttons on the PCB have been pushed, save the new time to the RTC.
      if (timeObj.time_update_needed){
        timeObj.performRTCUpdate();
        //show update on LCD
        lcd.clear();
        delay(1);
        lcd.home();
        lcd.write_text(" Current Time:");
        lcd.goto_xy(0,1);
        lcd.write_text(" " + timeObj.getDateForLCD() + " " + timeObj.getTimeForLCD());
        update_reg|=0b00000001;
        delay(700);
      }
      
      //Check serial object for incomming commands!
      serialObj.readNewData();
      //uint8_t tmparr[] = {222,222,221,221,220,220,1,2,3,4,5,6,7,8,9,10,11};
      uint8_t tmparr[] = {255,255,255,255};
      //COUNT_32=128;
      for (byte i=0; i<4; i++)//Store Total # of Pulses
         tmparr[i] = lowByte(COUNT_32>>(i*8));
      serialObj.transmitDataSmall(123,tmparr,4); 
      COUNT_32++;
      
      
    }  
    
    //************** Refresh time/date from RTC **********************
    if((update_reg&0x10)==16)
    {
      update_reg = update_reg&(~0x10); //clear flag
      timeObj.updateSystemClock();
    }
    

  //delay(50);
  }//End While(1)
}//END MAIN







/***************************************
* ISR - Functions called by ISR routine
*
****************************************/
//Falling edge trigger. Increase pulse counts 
void FlowPulseCount1(void){
 flowObj_1.incPulseCnt();
}

//Falling edge trigger. Increase pulse counts 
void FlowPulseCount2(void){
 //increase pulse counts 
 flowObj_2.incPulseCnt();
}

//Handles speed input counter
void SpeedPulseISR(void){
   //speed_pulses++;
   speedObj.incPulseCnt();
}

//Handles Hour Inc button to change RTC.
void TimeIncHhISR(void){
   timeObj.incHr();
}

//Handles Min Inc button to change RTC.
void TimeIncMmISR(void){
   timeObj.incMin();
}


//100ms system timer.
void MyTimer1ISR(void){
  volatile static byte  lcd_cnt;
  volatile static byte  fuel_cnt;
  volatile static byte  other_sigs_cnt;
  volatile static int   log_cnt;
  volatile static int   rtc_cnt;
  
  //Send LCD Update
  if(lcd_cnt>=TIME_LCD_UPDATE){
    update_reg|=0b00000001;
    lcd_cnt=0;}
  else
    lcd_cnt++;
  
  //Update Fuel Value
  if(fuel_cnt>=TIME_FUEL_AVERAGE){
    update_reg|=0b00000010;
    fuel_cnt=0;}
  else
    fuel_cnt++;
  
  //log interval
  if(log_cnt>=TIME_LOG_UPDATE){
    update_reg|=0b00000100;
    log_cnt=0;}
  else
    log_cnt++;  
    
  //misc. signal check interval
  if(other_sigs_cnt>=TIME_SIG_UPDATE){
    update_reg|=0b00001000;
    other_sigs_cnt=0;}
  else
    other_sigs_cnt++;     
 
  //RTC refresh
  if(rtc_cnt>=TIME_RTC_REFRESH){
    update_reg|=0b00010000;
    rtc_cnt=0;}
  else
    rtc_cnt++;    
 
    
}

/********************************* Supporing Funcitons ***********************************
*****************************************************************************************/


void cmdHandler(){
  uint8_t msg[256];
  uint8_t msg_info[4];
  
  //NOTE, not useing crc right now!
  serialObj.getAndValadateCommand(msg, msg_info);
  
  //If valid length message. Do somthing!
  if(msg_info[serialObj.MSG_INFO_LEN] != 0){
    
    //if(msg[1])
    //debug Attempt
    uint8_t trial_data[] = {0x08};
    //serialObj.transmitDataSmall(uint8_t(0x07), trial_data, uint8_t(0x01));
    
    //Example 
    
    //ACK
  }
  
}
      

/****************************************************
 Checks for button press with debounce.
*******************************************************/
boolean button_pushed(byte pin, byte active_level){
  if (digitalRead(pin)==active_level){
    delay(2);
    if (digitalRead(pin)==active_level)
      return true;
  }
  return false;
}



/****************************************************
 Checks Vin for brownout condition
 Last check this code is ~ 100us execution time if voltage ok.
*******************************************************/
void CheckBrownoutAndBackup(void){
  input_voltage = getInputVoltage();
  if (input_voltage<BROWNOUT_VOLTAGE){
    lcd.bkLightOff();
    savePulsesToEEPROM();
    delay(1000);
    lcd.bkLightOn();
  }
    
  
}




/****************************************************
 Loads system life count from eeprom
*******************************************************/
void loadPulsesFromEEPROM(void){
  unsigned long temp_pulses = 0;
  //Load 4 Bytes of data from EEPROM for each flow meter.
  for (byte i=0; i<TOTAL_PULSE_COUNT_LENGTH; i++)
    temp_pulses += EEPROM.read(TOTAL_PULSE_COUNT_BASE_ADDR_1+i)<<(i*8);  //store 1 byte * 4, shift in 3 upper bits
  flowObj_1.pulse_count_total = temp_pulses;
  temp_pulses = 0;
  for (byte i=0; i<TOTAL_PULSE_COUNT_LENGTH; i++)
    temp_pulses += EEPROM.read(TOTAL_PULSE_COUNT_BASE_ADDR_2+i)<<(i*8);  //store 1 byte * 4, shift in 3 upper bits
  flowObj_2.pulse_count_total = temp_pulses;
}

/****************************************************
 Saves scurrent flow count to eeprom
*******************************************************/
void savePulsesToEEPROM(void){
  //unsigned long temp_pulses = 0;
  for (byte i=0; i<TOTAL_PULSE_COUNT_LENGTH; i++)//Store Total # of Pulses
    EEPROM.write(TOTAL_PULSE_COUNT_BASE_ADDR_1+i, lowByte(flowObj_1.pulse_count_total>>(i*8)));  //write 1 byte * 4, shift in 3 upper bits 
  for (byte i=0; i<TOTAL_PULSE_COUNT_LENGTH; i++)//Store Total # of Pulses
    EEPROM.write(TOTAL_PULSE_COUNT_BASE_ADDR_2+i, lowByte(flowObj_2.pulse_count_total>>(i*8)));  //write 1 byte * 4, shift in 3 upper bits 
}


/***************************************
* Input 12bit ADC, Output Temp in *C
* [+3.3V] -----[Thermistor]-----|-------[10k-Resister]----- [GND]
* Uses Steinhart-Hart Thermistor Equation to linirize
* B:3984, Digikey:BC2741-ND    (accurate ~0.5*C based on specs. not counting ADC error)
****************************************/
float Thermistor(int RawADC, float shunt_resistor) {
   // Temperature in Kelvin = 1 / {A + B[ln(R)] + C[ln(R)]^3}
   // where A = 0.0011384572, B = 0.00023265334 and C = 0.000000093102687
   long Resistance;  double Temp;  // Dual-Purpose variable to save space.
   Resistance=((40960000/(RawADC)) - shunt_resistor);  // Assuming a 10k Thermistor.  Calculation is actually: Resistance = (1024 * BalanceResistor/ADC) - BalanceResistor
   Temp = log(Resistance); // Saving the Log(resistance) so not to calculate it 4 times later. // "Temp" means "Temporary" on this line.
   Temp = (1 / (113.84572 + (23.265334 * Temp) + (0.0093102687* Temp * Temp * Temp)))*100000;   // Now it means both "Temporary" and "Temperature"
   Temp = Temp - 273.15;  // Convert Kelvin to Celsius                                         // Now it only means "Temperature"
   //Temp = (Temp * 9.0)/ 5.0 + 32.0; // Convert to Fahrenheit
   return Temp;  // Return the Temperature
}



float ThermistorAltComp(int RawADC, float shunt_resistor) {
   long Resistance;  double Temp;  // Dual-Purpose variable to save space.
   double num; double den; double log_r;
   
   Resistance=((40960000/(RawADC)) - shunt_resistor);  // Assuming a 10k Thermistor.  Calculation is actually: Resistance = (1024 * BalanceResistor/ADC) - BalanceResistor
   log_r = log(10000/Resistance);
   num=float(298*3984.0)/log_r;
   den = float(3984.0/log_r)-298;
   Temp = float(num) / den;
   Temp = Temp - 273.15;  // Convert Kelvin to Celsius                                         // Now it only means "Temperature"
   return Temp;  // Return the Temperature
}




/****************************************************
 Gets input voltage
*******************************************************/
float getInputVoltage(void){
  float voltage;
  voltage = analogRead(PIN_INPUT_VOLT)/4095.0*3.3;
  return voltage * (26.7/4.7);
}


/****************************************************
 Prints inital screen
*******************************************************/
void startupMessage(void){
  LCDlib lcd(I2C_ADDR_LCD);
  delay(10);
  lcd.clear();
  lcd.home();
  lcd.write_text(" RT-E - ");
  lcd.write_text(SERIAL_NUMBER);
  lcd.goto_xy(0,1);
  lcd.write_text(" FW: ");
  lcd.write_text(SW_REV);
  lcd.write_text(" Vin: ");
  lcd.write_float1(input_voltage);
  lcd.goto_xy(0,2);
  lcd.write_text(" " + timeObj.getDateForLCD() + " " + timeObj.getTimeForLCD());
  lcd.goto_xy(0,3);
  lcd.write_text(" SD Card Status: ");
  if(sd_card_ok)
    lcd.write_text("OK");
  else
    lcd.write_text("ERR");
}  

/****************************************************
 Converts float to string with ## percision
*******************************************************/
String Float2String(float num, byte percision){
  boolean neg = false;
  String Output_d = "";
  String Output_i = "";
  if (num < 0){
    neg = true;
    num = -num;
  }
  
  long intPart = num;
  long fracPart = (num - intPart) * pow(10,percision+1);
  fracPart = (fracPart+5)/10;
  if(fracPart<10)//padd with zeros
    Output_d = "0" + String(fracPart); 
  else
    Output_d = String(fracPart); 
  Output_i = String(intPart); 
  if (neg)
    return ("-" + Output_i + "." + Output_d);
  else
    return (Output_i + "." + Output_d);
}
