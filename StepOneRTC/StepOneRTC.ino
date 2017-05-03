// StepOneRTC
// Energia I2C access to an MCP7940 RTC

#include <Wire.h>

const byte RTC_I2C_ADDR = 0x6F;
const byte RTC_ST_SEC = 0x00;
const byte RTC_MINS   = 0x01;
const byte RTC_HRS    = 0x02;
const byte RTC_DAY    = 0x03;
const byte RTC_DATE   = 0x04;
const byte RTC_MONTH  = 0x05;
const byte RTC_YEAR   = 0x06;
const byte RTC_CONTROL= 0x07;

byte mybuff[9]; // Working data for forming command sequences
byte value;
int  theActualValue;

void setup()
{
  Wire.setModule(0); // Required to select MSP430G2553IN20 pins 14/15 for I2C
  Wire.begin(); // Initialize connection to I2C bus as master.
  // Disable RTC in order to allow for unambiguous set-up of time/date.
  mybuff[0]=RTC_ST_SEC; mybuff[1]=0x00;
  Wire.beginTransmission(RTC_I2C_ADDR);
  Wire.write(mybuff,2);
  Wire.endTransmission();
  // Set minutes, hours, day, date, month, year
  mybuff[0]=RTC_MINS; // Address to begin writing into RTC
  mybuff[1]=0x30;       // Minute count is 30 (BCD)
  mybuff[2]=0x10;       // 24hr format, hour is 10 
  mybuff[3]=0x06;       // Sixth day of the week
  mybuff[4]=0x25;       // 25th day of the year
  mybuff[5]=0x04;       // Fourth month of the year
  mybuff[6]=0x17;       // 17th year
  mybuff[7]=0x43;       // No alarms, square wave output at 32.768 kHz
  Wire.beginTransmission(RTC_I2C_ADDR);
  Wire.write(mybuff,8);
  Wire.endTransmission();
  // Enable RTC with initialized seconds at 37 (BCD-ish), see Datasheet
  mybuff[0]=RTC_ST_SEC; mybuff[1]=0xB7;
  Wire.beginTransmission(RTC_I2C_ADDR);
  Wire.write(mybuff,2);
  Wire.endTransmission();
  // Enable an ENERGIA Serial terminal
  Serial.begin(9600); 
}

void loop()
{
  Wire.beginTransmission(RTC_I2C_ADDR); 
  Wire.write(RTC_ST_SEC);             
  Wire.endTransmission();
  Wire.requestFrom(RTC_I2C_ADDR,1);
  value=Wire.read();
  theActualValue = (value & 0x0f) + ((value >> 4) & 0x07)*10;
  Serial.println(theActualValue);
  delay(500);
}
