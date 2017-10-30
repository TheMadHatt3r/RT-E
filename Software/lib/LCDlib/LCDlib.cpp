/*
  LCDlib.cpp - library for writing to a 20x4 Newhaven i2c LCD
  Created by Collin Matthews, 14 October 2013
  For use by Reicon Systems, all other uses prohibited without consent.
  ctmrocks2435@yahoo.com
*/

#include <Arduino.h>
#include <LCDlib.h>

//default constructor to fix scope problem
LCDlib::LCDlib()
{
}

LCDlib::LCDlib(int address)
{
  //addr = address >> 1;+
  addr = address;
}


void LCDlib::init()
{
  bkLightOn();
}

void LCDlib::clear()
{
  Wire.beginTransmission(addr);
  Wire.write(0xFE);
  Wire.write(0x51);
  Wire.endTransmission();
  delay(2);
  return;
}

void LCDlib::home()
{
  Wire.beginTransmission(addr);
  Wire.write(0xFE);
  Wire.write(0x46);
  Wire.endTransmission();
  delay(2);
  return;
}

void LCDlib::bkLightOn()
{
  backlight(2);
  Wire.beginTransmission(addr);
  Wire.write(0xFE);
  Wire.write(0x41);
  Wire.endTransmission();
  
  return;
}

void LCDlib::bkLightOff()
{
  Wire.beginTransmission(addr);
  Wire.write(0xFE);
  Wire.write(0x42);
  Wire.endTransmission();
  backlight(1);
  return;
}

void LCDlib::write_text(char x[])
{
  Wire.beginTransmission(addr);
  Wire.write(x);
  Wire.endTransmission();
  return;
}

void LCDlib::write_text(String x)
{
  char tmp2[x.length() + 1];
  x.toCharArray(tmp2, x.length() + 1);
  Wire.beginTransmission(addr);
  Wire.write(tmp2);
  Wire.endTransmission();
  return;
}

void LCDlib::write_int(int a)
{
  String tmp = String(a);
  char tmp2[tmp.length() + 1];
  tmp.toCharArray(tmp2, tmp.length() + 1);
  Wire.beginTransmission(addr);
  Wire.write(tmp2);
  Wire.endTransmission();
  return;
}

void LCDlib::write_ulong(unsigned long a)
{
  String tmp = String(a);
  char tmp2[tmp.length() + 1];
  tmp.toCharArray(tmp2, tmp.length() + 1);
  Wire.beginTransmission(addr);
  Wire.write(tmp2);
  Wire.endTransmission();
  return;
}

void LCDlib::write_tm(int a)
{
  if (a >= 0 && a < 10) //single digit
  {
  String tmp = "0" + String(a);
  char tmp2[tmp.length() + 1];
  tmp.toCharArray(tmp2, tmp.length() + 1);
  Wire.beginTransmission(addr);
  Wire.write(tmp2);
  Wire.endTransmission();
  }
  else
  {
  String tmp = String(a);
  char tmp2[tmp.length() + 1];
  tmp.toCharArray(tmp2, tmp.length() + 1);
  Wire.beginTransmission(addr);
  Wire.write(tmp2);
  Wire.endTransmission();
  }
  return;
}


//Writes a float with 2 decimals of precision
void LCDlib::write_float2(float b)
{
  boolean neg = false;
  String tmp = "";
  
  //check if negative
  if (b<0){
	neg = true;
	b = -b;
  }
  
  int whole = int(b);
  int dec = int(b * 100 - whole * 100);
  String dec_string = String(dec);
  if (dec >= 0 && dec < 10) //single digit
	dec_string = "0" + String(dec);
  
  if (neg)
	tmp = "-" + String(whole) + "." + dec_string;
  else
    tmp = String(whole) + "." + dec_string;
  char tmp2[tmp.length() + 1];
  tmp.toCharArray(tmp2, tmp.length() + 1);
  Wire.beginTransmission(addr);
  Wire.write(tmp2);
  Wire.endTransmission();
  return;
}

//Writes a float with 1 decimals of precision
void LCDlib::write_float1(float b)
{
  boolean neg = false;
  String tmp = "";
  
  //check if negative
  if (b<0){
	neg = true;
	b = -b;
  }
	
  int whole = int(b);
  int dec = int(b * 10 - whole * 10);
  String dec_string = String(dec);
  if (neg)
	tmp = "-" + String(whole) + "." + dec_string;
  else
    tmp = String(whole) + "." + dec_string;
  char tmp2[tmp.length() + 1];
  tmp.toCharArray(tmp2, tmp.length() + 1);
  Wire.beginTransmission(addr);
  Wire.write(tmp2);
  Wire.endTransmission();
  return;
}

void LCDlib::goto_xy(int x, int y)
{
  // X:0 to 19, Y:0 to 3 -- 20x4 LCD
  switch(y)
  {
   case 0:
    Wire.beginTransmission(addr);
    Wire.write(0xFE);
    Wire.write(0x45);
    Wire.write(x);
    Wire.endTransmission();
   break;
  
  case 1:  
    Wire.beginTransmission(addr);
    Wire.write(0xFE);
    Wire.write(0x45);
    Wire.write(64 + x);
    Wire.endTransmission();
  break;
  
  case 2:  
    Wire.beginTransmission(addr);
    Wire.write(0xFE);
    Wire.write(0x45);
    Wire.write(20 + x);
    Wire.endTransmission();
  break;
  
  case 3:
    Wire.beginTransmission(addr);
    Wire.write(0xFE);
    Wire.write(0x45);
    Wire.write(84 + x);
    Wire.endTransmission();
  break;
  }
  delay(1);
  return;
}

void LCDlib::backlight(int l)
{
    Wire.beginTransmission(addr);
    Wire.write(0xFE);
    Wire.write(0x53);
    Wire.write(l);
    Wire.endTransmission();
    //delay(1);
    return;
}

void LCDlib::contrast(int c)
{
    Wire.beginTransmission(addr);
    Wire.write(0xFE);
    Wire.write(0x52);
    Wire.write(c);
    Wire.endTransmission();
    delay(2);
    return;
}