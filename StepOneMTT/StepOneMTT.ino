// StepOneMTT.ino
// Energia I2C access to a Maxim DS1631 Temp module.

#include <Wire.h>

const byte MTT_I2C_ADDR = 0x48;
const byte MTT_CMD_START_CONVERT = 0x51;
const byte MTT_CMD_STOP_CONVERT  = 0x22;
const byte MTT_CMD_READ_TEMP     = 0xAA;
const byte MTT_CMD_ACCESS_TH     = 0xA1;
const byte MTT_CMD_ACCESS_TL     = 0xA2;
const byte MTT_CMD_ACCESS_CONFIG = 0xAC;
const byte MTT_CMD_SOFTWARE_POR  = 0x54;
const byte MTT_CONFIG_DEFAULT    = 0x8C;

byte mybuff[3]; // Working data for forming command sequences
byte RwholeValue;
byte RfractionValue;
int  fracC;
byte wholeF;
int  fracF;
char msgBuffer[40];

// MTT second byte is all fractional in C
// This converts up to four MSbs to parts-per-ten-thousand
// (that is, theValue/10000 is the numerical fractional part in C)
int convertFractionalPart(byte fC) {
  int theValue=0;
  int AMOUNT=5000;
  for(int i=0; i<4; i++) {
    if(0x80 & fC)
      theValue += AMOUNT;
    AMOUNT >>= 1;
    fC <<= 1;
  }
  return theValue;
}

// Convert the MTT raw C byte and fraction to F
void convertCtoF(byte wC, int fC, byte *wF, int *fF ) {
  long int CK, FK;
  CK = wC*10000L+fC;
  FK = (9*CK)/5+320000L;
  *wF = FK/10000L;
  *fF = FK%10000L;
}

void setup()
{
  Wire.setModule(0);
  Wire.begin(); // Initialize connection to I2C bus as master.
  // Set MTT to the factory default configuration.
  mybuff[0]=MTT_CMD_ACCESS_CONFIG; mybuff[1]=MTT_CONFIG_DEFAULT;
  Wire.beginTransmission(MTT_I2C_ADDR);
  Wire.write(mybuff,2);
  Wire.endTransmission();
  // Tell MTT to begin converting temperatures
  Wire.beginTransmission(MTT_I2C_ADDR);
  Wire.write(MTT_CMD_START_CONVERT);
  Wire.endTransmission();
  // Enable an ENERGIA Serial terminal
  Serial.begin(9600); 
}

void loop()
{
  Wire.beginTransmission(MTT_I2C_ADDR); 
  Wire.write(MTT_CMD_READ_TEMP);             
  Wire.endTransmission();
  Wire.requestFrom(MTT_I2C_ADDR,2);
  RwholeValue=Wire.read();
  RfractionValue=Wire.read();
  fracC=convertFractionalPart(RfractionValue);
  convertCtoF(RwholeValue,fracC,&wholeF,&fracF);
  sprintf(msgBuffer,"Reading: 0x%02x%02x R / %d.%04d C / %d.%04d F",
          RwholeValue,RfractionValue,RwholeValue,convertFractionalPart(RfractionValue),
          wholeF,fracF);
  Serial.println(msgBuffer);
  delay(500);
}
