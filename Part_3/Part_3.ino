// include the library code:
#include <LiquidCrystal.h>
#include <Wire.h>

// initialize the library with the numbers of the interface pins
LiquidCrystal lcd(P2_0, P2_1, P2_2, P2_3, P2_4, P2_5);

const byte MTT_I2C_ADDR = 0x48;
const byte MTT_CMD_START_CONVERT = 0x51;
const byte MTT_CMD_STOP_CONVERT  = 0x22;
const byte MTT_CMD_READ_TEMP     = 0xAA;
const byte MTT_CMD_ACCESS_TH     = 0xA1;
const byte MTT_CMD_ACCESS_TL     = 0xA2;
const byte MTT_CMD_ACCESS_CONFIG = 0xAC;
const byte MTT_CMD_SOFTWARE_POR  = 0x54;
const byte MTT_CONFIG_DEFAULT    = 0x8C;

const byte RTC_I2C_ADDR = 0x6F;
const byte RTC_ST_SEC = 0x00;
const byte RTC_MINS   = 0x01;
const byte RTC_HRS    = 0x02;
const byte RTC_DAY    = 0x03;
const byte RTC_DATE   = 0x04;
const byte RTC_MONTH  = 0x05;
const byte RTC_YEAR   = 0x06;
const byte RTC_CONTROL= 0x07;

const char DayOfWeek[7][4] =  {"Mon","Tue","Wed","Thu","Fri",
                               "Sat","Sun"};
const char MonthName[12][4] = {"Jan","Feb","Mar","Apr","May",
                               "Jun","Jul","Aug","Sep","Oct",
                               "Nov","Dec"};

byte mybuff[9]; // Working data for forming command sequences
byte value;

byte mybuffTemp[3]; // Working data for forming command sequences
byte RwholeValue;
byte RfractionValue;
int  fracC;
byte wholeF;
int  fracF;
char Temp[7];

char Seconds[2];
char Minutes[2];
char Hours[2];
char Day[2];

int  CorF = 0;

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

void convertCtoF(byte wC, int fC, byte *wF, int *fF ) {
  long int CK, FK;
  CK = wC*10000L+fC;
  FK = (9*CK)/5+320000L;
  *wF = FK/10000L;
  *fF = FK%10000L;
}

int convertToDec(byte value)
{
  return (value & 0x0f) + ((value & 0x70) >> 4) *10;
}

void setup() {
  
  //Set up the RTC
  //---------------------------------------------------------------------------------------
  
  Wire.setModule(0); // Required to select MSP430G2553IN20 pins 14/15 for I2C
  Wire.begin(); // Initialize connection to I2C bus as master.
  // Disable RTC in order to allow for unambiguous set-up of time/date.
  mybuff[0]=RTC_ST_SEC; mybuff[1]=0x00;
  Wire.beginTransmission(RTC_I2C_ADDR);
  Wire.write(mybuff,2);
  Wire.endTransmission();
  // Set minutes, hours, day, date, month, year
  mybuff[0]=RTC_MINS; // Address to begin writing into RTC
  mybuff[1]=0x59;       // Minute count is 30 (BCD)
  mybuff[2]=0x23;       // 24hr format, hour is 23 
  mybuff[3]=0x06;       // Seventh day of the week
  mybuff[4]=0x31;       // 31th day of the month
  mybuff[5]=0x12;       // Twelfth month of the year
  mybuff[6]=0x17;       // 17th year
  mybuff[7]=0x43;       // No alarms, square wave output at 32.768 kHz
  Wire.beginTransmission(RTC_I2C_ADDR);
  Wire.write(mybuff,8);
  Wire.endTransmission();
  // Enable RTC with initialized seconds at 47 (BCD-ish), see Datasheet
  mybuff[0]=RTC_ST_SEC; mybuff[1]=0xD5;
  Wire.beginTransmission(RTC_I2C_ADDR);
  Wire.write(mybuff,2);
  Wire.endTransmission();


 //Set up the Thermometer
  //---------------------------------------------------------------------------------------
  
  Wire.setModule(0);
  Wire.begin(); // Initialize connection to I2C bus as master.
  // Set MTT to the factory default configuration.
  mybuffTemp[0]=MTT_CMD_ACCESS_CONFIG; mybuffTemp[1]=MTT_CONFIG_DEFAULT;
  Wire.beginTransmission(MTT_I2C_ADDR);
  Wire.write(mybuffTemp,2);
  Wire.endTransmission();
  // Tell MTT to begin converting temperatures
  Wire.beginTransmission(MTT_I2C_ADDR);
  Wire.write(MTT_CMD_START_CONVERT);
  Wire.endTransmission();


  //Set up the LCD and button
  //-----------------------------------------------------------------------------------------
  
  lcd.begin(16, 2);
  pinMode(P1_3, INPUT_PULLUP);
}

void loop() {
  //Take care of the RTC
  //---------------------------------------------------------------------------------------


 // HOURS
//--------------------------------------------------------------------------------

  Wire.beginTransmission(RTC_I2C_ADDR); 
  Wire.write(RTC_HRS);             
  Wire.endTransmission();
  Wire.requestFrom(RTC_I2C_ADDR,1);
  value=Wire.read();

  sprintf(Hours,"%02d",convertToDec(value));
  lcd.setCursor(0, 0);
  lcd.print(Hours);

// MINUTES
//--------------------------------------------------------------------------------

  lcd.setCursor(2, 0);
  lcd.print(":"); 
  
  Wire.beginTransmission(RTC_I2C_ADDR); 
  Wire.write(RTC_MINS);             
  Wire.endTransmission();
  Wire.requestFrom(RTC_I2C_ADDR,1);
  value=Wire.read();

  sprintf(Minutes,"%02d",convertToDec(value));
  lcd.setCursor(3, 0);
  lcd.print(Minutes);

// SECONDS
//--------------------------------------------------------------------------------
  
  lcd.setCursor(5, 0);
  lcd.print(":");

  Wire.beginTransmission(RTC_I2C_ADDR); 
  Wire.write(RTC_ST_SEC);             
  Wire.endTransmission();
  Wire.requestFrom(RTC_I2C_ADDR,1);
  value=Wire.read();
  
  sprintf(Seconds,"%02d",convertToDec(value));
  lcd.setCursor(6, 0);
  lcd.print(Seconds);
  
// MONTH
//--------------------------------------------------------------------------------
  Wire.beginTransmission(RTC_I2C_ADDR); 
  Wire.write(RTC_MONTH);             
  Wire.endTransmission();
  Wire.requestFrom(RTC_I2C_ADDR,1);
  value=Wire.read();

  lcd.setCursor(0, 1);
  lcd.print(MonthName[convertToDec(value)-1]); 

// DAY
//---------------------------------------------------------------------------------------
  
  lcd.setCursor(3, 1);
  lcd.print("-");
  
  Wire.beginTransmission(RTC_I2C_ADDR); 
  Wire.write(RTC_DATE);             
  Wire.endTransmission();
  Wire.requestFrom(RTC_I2C_ADDR,1);
  value=Wire.read();

  sprintf(Day,"%02d",convertToDec(value));
  lcd.setCursor(4, 1);
  lcd.print(Day);  

//YEAR
//---------------------------------------------------------------------------------------
  
  lcd.setCursor(6, 1);
  lcd.print("-");
  
  Wire.beginTransmission(RTC_I2C_ADDR); 
  Wire.write(RTC_YEAR);             
  Wire.endTransmission();
  Wire.requestFrom(RTC_I2C_ADDR,1);
  value=Wire.read();
  
  lcd.setCursor(7, 1);
  lcd.print("20");
  lcd.print(convertToDec(value));  

// DAY OF WEEK
//---------------------------------------------------------------------------------------
   
  Wire.beginTransmission(RTC_I2C_ADDR); 
  Wire.write(RTC_DAY);             
  Wire.endTransmission();
  Wire.requestFrom(RTC_I2C_ADDR,1);
  value=Wire.read();

  lcd.setCursor(12, 1);
  lcd.print(DayOfWeek[(value & 0x0f)-1]);
  
//||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||


  //Take care of the thermometer
  //-------------------------------------------------------------------------------------
  Wire.beginTransmission(MTT_I2C_ADDR); 
  Wire.write(MTT_CMD_READ_TEMP);             
  Wire.endTransmission();
  Wire.requestFrom(MTT_I2C_ADDR,2);
  RwholeValue=Wire.read();
  RfractionValue=Wire.read();
  fracC=convertFractionalPart(RfractionValue);
  convertCtoF(RwholeValue,fracC,&wholeF,&fracF);

  if(digitalRead(P1_3) == LOW)
  {
    CorF = !CorF;
    delay(500);
  }
  
  if(CorF == 1)
  { 
    sprintf(Temp,"%d.%01d C",RwholeValue,round(convertFractionalPart(RfractionValue)/1000));
    lcd.setCursor(10, 0);
    lcd.print(Temp);
  }
  else
  {
    sprintf(Temp,"%d.%01d F",wholeF,round(fracF/1000));
    lcd.setCursor(10, 0);
    lcd.print(Temp);
  }
  delay(50);
}
