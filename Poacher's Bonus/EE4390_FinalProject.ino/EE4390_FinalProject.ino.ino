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

const char DayOfWeek[7][4] =  {"Sun","Mon","Tue","Wed","Thu","Fri",
                               "Sat"};
const char MonthName[12][4] = {"Jan","Feb","Mar","Apr","May",
                               "Jun","Jul","Aug","Sep","Oct",
                               "Nov","Dec"};

byte mybuff[9]; // Working data for forming command sequences
byte value;
byte leapYear = 0, monthLength = 0;
byte oneTimeAlarm[7];
byte dailyAlarm[3];

byte mybuffTemp[3]; // Working data for forming command sequences
byte RwholeValue;
byte RfractionValue;
int  fracC;
byte wholeF;
int  fracF;
char Temp[7];
byte state;
byte settingAlarm;
byte blinkLED;
bool LEDon;

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
  return (value & 0x0f) + ((value & 0xf0) >> 4) *10;
}

byte convertToHexish(byte value)
{
  return (value/10)<<4 | (value%10);
}

void setup() {


    Wire.beginTransmission(RTC_I2C_ADDR); 
    Wire.write(RTC_DAY);             
    Wire.endTransmission();
    Wire.requestFrom(RTC_I2C_ADDR,1);
    value=Wire.read();
    
  //Set up the RTC
  //---------------------------------------------------------------------------------------
  if(value != 0x19)
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
  mybuff[1]=0x00;       // Minute count is 00 (BCD)
  mybuff[2]=0x00;       // 24hr format, hour is 00 
  mybuff[3]=0x09;       // First day of the week, VBATEN Set
  mybuff[4]=0x01;       // 1st day of the month
  mybuff[5]=0x01;       // 1st month of the year
  mybuff[6]=0x01;       // year 2001
  mybuff[7]=0x43;       // No alarms, square wave output at 32.768 kHz
  Wire.beginTransmission(RTC_I2C_ADDR);
  Wire.write(mybuff,8);
  Wire.endTransmission();
  // Enable RTC with initialized seconds at 00 (BCD-ish), see Datasheet
  mybuff[0]=RTC_ST_SEC; mybuff[1]=0x80;
  Wire.beginTransmission(RTC_I2C_ADDR);
  Wire.write(mybuff,2);
  Wire.endTransmission();
  }

  else
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
  mybuff[1]=0x05;       // Minute count is 00 (BCD)
  mybuff[2]=0x05;       // 24hr format, hour is 00 
  mybuff[3]=0x0A;       // First day of the week, VBATEN Set
  mybuff[4]=0x05;       // 1st day of the month
  mybuff[5]=0x05;       // 1st month of the year
  mybuff[6]=0x05;       // year 2001
  mybuff[7]=0x43;       // No alarms, square wave output at 32.768 kHz
  Wire.beginTransmission(RTC_I2C_ADDR);
  Wire.write(mybuff,8);
  Wire.endTransmission();
  // Enable RTC with initialized seconds at 00 (BCD-ish), see Datasheet
  mybuff[0]=RTC_ST_SEC; mybuff[1]=0x80;
  Wire.beginTransmission(RTC_I2C_ADDR);
  Wire.write(mybuff,2);
  Wire.endTransmission();
  }


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


  //Set up the LCD, LED, and buttons
  //-----------------------------------------------------------------------------------------
  
  lcd.begin(16, 2);
  pinMode(P1_0, OUTPUT);
  pinMode(P1_3, INPUT_PULLUP);
  pinMode(P1_4, INPUT_PULLUP);

  digitalWrite(P1_0, LOW);
  //Set the initial state
  //-----------------------------------------------------------------------------------------
  state = 0;
  settingAlarm = 0;
  blinkLED = 0;
  LEDon = 1;
  oneTimeAlarm[0] = 0x80;
  oneTimeAlarm[1] = 0xff;
  dailyAlarm[0] = 0x80;
  dailyAlarm[1] = 0xff;
}

void loop() {
  if(state == 0)
  {
  //Take care of the RTC
  //---------------------------------------------------------------------------------------
    // HOURS
    //--------------------------------------------------------------------------------
    Wire.beginTransmission(RTC_I2C_ADDR); 
    Wire.write(RTC_HRS);             
    Wire.endTransmission();
    Wire.requestFrom(RTC_I2C_ADDR,1);
    value=Wire.read();
    mybuff[2] = value;

    sprintf(Temp,"%02d",convertToDec(value));
    lcd.setCursor(0, 0);
    lcd.print(Temp);

    // MINUTES
    //--------------------------------------------------------------------------------
    lcd.setCursor(2, 0);
    lcd.print(":"); 
  
    Wire.beginTransmission(RTC_I2C_ADDR); 
    Wire.write(RTC_MINS);             
    Wire.endTransmission();
    Wire.requestFrom(RTC_I2C_ADDR,1);
    value=Wire.read();
    mybuff[1] = value;

    sprintf(Temp,"%02d",convertToDec(value));
    lcd.setCursor(3, 0);
    lcd.print(Temp);

    // SECONDS
    //--------------------------------------------------------------------------------
    lcd.setCursor(5, 0);
    lcd.print(":");

    Wire.beginTransmission(RTC_I2C_ADDR); 
    Wire.write(RTC_ST_SEC);             
    Wire.endTransmission();
    Wire.requestFrom(RTC_I2C_ADDR,1);
    value=Wire.read();
    mybuff[0] = value;
  
    sprintf(Temp,"%02d",(value & 0x0f) + ((value & 0x70) >> 4) *10);
    lcd.setCursor(6, 0);
    lcd.print(Temp);
  
    // MONTH
    //--------------------------------------------------------------------------------
    Wire.beginTransmission(RTC_I2C_ADDR); 
    Wire.write(RTC_MONTH);             
    Wire.endTransmission();
    Wire.requestFrom(RTC_I2C_ADDR,1);
    value=Wire.read();
    mybuff[5] = value;

    lcd.setCursor(0, 1);
    lcd.print(MonthName[ convertToDec(0x1f & value) - 1] ); 

    // DAY
    //---------------------------------------------------------------------------------------
    lcd.setCursor(3, 1);
    lcd.print("-");
  
    Wire.beginTransmission(RTC_I2C_ADDR); 
    Wire.write(RTC_DATE);             
    Wire.endTransmission();
    Wire.requestFrom(RTC_I2C_ADDR,1);
    value=Wire.read();
    mybuff[4] = value;

    sprintf(Temp,"%02d",convertToDec(value));
    lcd.setCursor(4, 1);
    lcd.print(Temp);  

    //YEAR
    //---------------------------------------------------------------------------------------
    lcd.setCursor(6, 1);
    lcd.print("-");
  
    Wire.beginTransmission(RTC_I2C_ADDR); 
    Wire.write(RTC_YEAR);             
    Wire.endTransmission();
    Wire.requestFrom(RTC_I2C_ADDR,1);
    value=Wire.read();
    mybuff[6] = value;
  
    lcd.setCursor(7, 1);
    sprintf(Temp,"20%02d",convertToDec(value));
    lcd.print(Temp);  

    // DAY OF WEEK
    //---------------------------------------------------------------------------------------
    Wire.beginTransmission(RTC_I2C_ADDR); 
    Wire.write(RTC_DAY);             
    Wire.endTransmission();
    Wire.requestFrom(RTC_I2C_ADDR,1);
    value=Wire.read();

    lcd.setCursor(12, 1);
    lcd.print(DayOfWeek[(value & 0x07)-1]);

    // CHECK FOR ALARM
    //---------------------------------------------------------------------------------------

    // Check against One-time Alarm
    for(value = 0; value < 7; value++)
    {
      if(value != 3 && mybuff[value] != oneTimeAlarm[value])
      {
        break;
      }
    }
    if(value == 7)
    {
      blinkLED = 1;
    }

    // Check against Daily Alarm
    for(value = 0; value < 3; value++)
    {
      if(mybuff[value] != dailyAlarm[value])
      {
        break;
      }
    }
    if(value == 3)
    {
      blinkLED = 1;
    }

    // Blink the LED if either alarm is activated
    if(blinkLED == 1)
    {
      LEDon = !LEDon;
    }
    if(LEDon)
    {
      digitalWrite(P1_0, LOW);
    }
    else
    {
      digitalWrite(P1_0, HIGH);
    }
    
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

    // Check for request to change temp reading or disable alarm
    if(digitalRead(P1_3) == LOW)
    {
      delay(500);
      state = 16;
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

    // Check for request to set starting values or alarm
    if(digitalRead(P1_4) == LOW)
    {
      delay(500);
      state = 14;
    }

    delay(50);
  }
  else if(state == 1) // GET YEAR
  {
    Wire.beginTransmission(RTC_I2C_ADDR); 
    Wire.write(RTC_YEAR);             
    Wire.endTransmission();
    Wire.requestFrom(RTC_I2C_ADDR,1);
    value=convertToDec(Wire.read());
  
    lcd.clear();
    lcd.setCursor(0, 0);
    sprintf(Temp,"Set year: 20%02d",value);
    lcd.print(Temp);
    state++;
  }
  else if(state == 2) // SET YEAR
  {
    lcd.setCursor(12,0);
    if(digitalRead(P1_3) == LOW)
    {
      if(value < 99)
        value++;
      else
        value = 1;
        
      sprintf(Temp,"%02d",value);
      lcd.print(Temp);
      delay(200);
    }
    // Check for request to set starting values
    if(digitalRead(P1_4) == LOW)
    {
      state++;
      
      if(value%4 == 0)
        leapYear = 1;
      else
        leapYear = 0;
        
      mybuff[6]=convertToHexish(value);
      delay(500);
    }
  }
  else if(state == 3) // GET MONTH
  {
    Wire.beginTransmission(RTC_I2C_ADDR); 
    Wire.write(RTC_MONTH);             
    Wire.endTransmission();
    Wire.requestFrom(RTC_I2C_ADDR,1);
    value=convertToDec(Wire.read() & 0x1f);
    
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Set month: ");
    lcd.print(MonthName[value-1]);
    state++;
  }
  else if(state == 4) // SET MONTH
  {
    lcd.setCursor(11,0);
    
    if(digitalRead(P1_3) == LOW)
    {
      if(value < 12)
        value++;
      else
        value = 1;
      lcd.print(MonthName[value-1]);
      delay(400);
    }
    
    // Check for request to set starting values
    if(digitalRead(P1_4) == LOW)
    {
      if(value == 2)
      {
        if(leapYear == 1)
          monthLength = 29;
        else
          monthLength = 28;
      }
      else if(value == 9 || value == 4 || value == 6 || value == 11)
        monthLength = 30;
      else
        monthLength = 31;
        
      state++;
      if(leapYear == 1)
        mybuff[5] = 0x20 | convertToHexish(value);
      else
        mybuff[5] = convertToHexish(value);
      delay(500);
    }
  }
  else if(state == 5) // GET DAY
  {
    Wire.beginTransmission(RTC_I2C_ADDR); 
    Wire.write(RTC_DATE);             
    Wire.endTransmission();
    Wire.requestFrom(RTC_I2C_ADDR,1);
    value=convertToDec(Wire.read());

    if(value > monthLength)
    {
      value = monthLength;
    }
  
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Set day: ");
    sprintf(Temp,"%02d",value);
    lcd.print(Temp);
    state++;
  }
  else if(state == 6) // SET DAY
  {
    lcd.setCursor(9,0);
    if(digitalRead(P1_3) == LOW)
    {
      if(value < monthLength)
        value++;
      else
        value = 1;
      sprintf(Temp,"%02d",value);
      lcd.print(Temp);
      delay(300);
    }
    
    // Check for request to set starting values
    if(digitalRead(P1_4) == LOW)
    {
      state++;
      mybuff[4]=convertToHexish(value);
      delay(500);
    }
  }
  else if(state == 7) // GET DAY OF THE WEEK
  {
    if(settingAlarm == 1)
    {
      state = 9;
    }
    else
    {
      Wire.beginTransmission(RTC_I2C_ADDR); 
      Wire.write(RTC_DAY);             
      Wire.endTransmission();
      Wire.requestFrom(RTC_I2C_ADDR,1);
      value=Wire.read() & 0x0f;
      lcd.setCursor(0, 1);
      lcd.print(value);
  
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Set Weekday: ");
      lcd.print(DayOfWeek[value-1]);
      state++;
    }
  }
  else if(state == 8) // SET DAY OF THE WEEK
  {
    lcd.setCursor(13,0);
    if(digitalRead(P1_3) == LOW)
    {
      if(value < 7)
        value++;
      else
        value = 1;
        
      lcd.print(DayOfWeek[value-1]);
      delay(500);
    }
    // Check for request to set starting values
    if(digitalRead(P1_4) == LOW)
    {
      state++;
      mybuff[3]=value & 0x0f;
      delay(500);
    }
  }
  else if(state == 9) // GET HOUR
  {
    Wire.beginTransmission(RTC_I2C_ADDR); 
    Wire.write(RTC_HRS);             
    Wire.endTransmission();
    Wire.requestFrom(RTC_I2C_ADDR,1);
    value=convertToDec(Wire.read());
  
    lcd.clear();
    lcd.setCursor(0, 0);
    sprintf(Temp,"Set hour: %02d",value);
    lcd.print(Temp);
    state++;
  }
  else if(state == 10) // SET HOUR
  {
    lcd.setCursor(10,0);
    if(digitalRead(P1_3) == LOW)
    {
      if(value < 23)
        value++;
      else
        value = 0;
      sprintf(Temp,"%02d",value);
      lcd.print(Temp);
      delay(400);
    }
    
    // Check for request to set starting values
    if(digitalRead(P1_4) == LOW)
    {
      state++;
      mybuff[2]=convertToHexish(value);
      delay(500);
    }
  }
  else if(state == 11) // GET MINUTES
  {
    Wire.beginTransmission(RTC_I2C_ADDR); 
    Wire.write(RTC_MINS);             
    Wire.endTransmission();
    Wire.requestFrom(RTC_I2C_ADDR,1);
    value=convertToDec(Wire.read());
  
    lcd.clear();
    lcd.setCursor(0, 0);
    sprintf(Temp,"Set minute: %02d",value);
    lcd.print(Temp);
     state++;
  }
  else if(state == 12) // SET MINUTES
  {
    lcd.setCursor(12,0);
    if(digitalRead(P1_3) == LOW)
    {
      if(value < 59)
        value++;
      else
        value = 0;
      sprintf(Temp,"%02d",value);
      lcd.print(Temp);
      delay(200);
    }
    
    // Check for request to set starting values
    if(digitalRead(P1_4) == LOW)
    {
      state++;
      mybuff[1]=convertToHexish(value);
      delay(500);
    }
  }
  else if(state == 13)
  {
    if(settingAlarm == 1)
    {
      for(value = 1; value < 7; value++)
      {
        oneTimeAlarm[value] = mybuff[value];
      }
    }
    else if(settingAlarm == 2)
    {
      for(value = 1; value < 3; value++)
      {
        dailyAlarm[value] = mybuff[value];
      }
    }
    else
    {
      // Set minutes, hours, day, date, month, year
      mybuff[0] = RTC_MINS;
      mybuff[7] = 0x43;
      Wire.beginTransmission(RTC_I2C_ADDR);
      Wire.write(mybuff,8);
      Wire.endTransmission();

      Wire.beginTransmission(RTC_I2C_ADDR);
      Wire.write(mybuff,8);
      Wire.endTransmission();
    }

    // Go back to clock and temp display
    settingAlarm = 0;
    lcd.clear();
    state = 0;
  }
  else if(state == 14) // Choose alarm or time
  {
    if(digitalRead(P1_4) == HIGH)
    {
        state = 1;
        settingAlarm = 0;
    }
    else if(digitalRead(P1_4) == LOW)
    {
      state = 15;
      lcd.clear();
      lcd.setCursor(0,0);
      lcd.print("Set Alarm Mode:");
      lcd.setCursor(0,1);
      lcd.print("Wait One Moment");
      delay(2000);
      lcd.clear();
      lcd.setCursor(0,0);
      lcd.print("Daily: black");
      lcd.setCursor(0,1);
      lcd.print("One-time: red");
    }
  }
  else if(state == 15) // Choose daily or single alarm
  {
    if(digitalRead(P1_4) == LOW)
    {
      state = 9;
      settingAlarm = 2;
      delay(500);
    }
    if(digitalRead(P1_3) == LOW)
    {
      state = 1;
      settingAlarm = 1;
      delay(500);
    }
  }
  else if(state == 16) // Turn off alarm or change temp unit
  {
    if(digitalRead(P1_3) == LOW)
    {
      blinkLED = 0;
      LEDon = 1;
      lcd.clear();
      lcd.setCursor(0,0);
      lcd.print("Turned Alarm Off");
      delay(2000);
      lcd.clear();
      delay(100);
    }
    else if(digitalRead(P1_3) == HIGH)
    {
      CorF = !CorF;
    }
    state = 0;
  }
}
