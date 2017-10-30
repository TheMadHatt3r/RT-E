#include <Arduino.h>
#include <Wire.h>
#include <LCDlib.h>

// Manually lower TWI_FREQ from 100000L to 60000L in twi.h
// 65000L seemed ok, but leaving room for error

void setup()
{
  delay(10);
  
  Wire.begin();
  
  LCDlib lcd(0x50); // Pass i2c address of LCD in constructor.  Will take care of bit shift automatically
  
  lcd.clear();
  lcd.home();
  lcd.write_text("HI");
  lcd.goto_xy(4,2);
  lcd.write_text("Hello");
  lcd.backlight(2);
  lcd.contrast(45);
}

void loop()
{
  
}
