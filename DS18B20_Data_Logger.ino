/*****************************************************************************
* Copyright (C) 2014-2015 by Vasco Ferraz. All Rights Reserved.              *
*                                                                            *
* This program is free software: you can redistribute it and/or modify       *
* it under the terms of the GNU General Public License as published by       *
* the Free Software Foundation, either version 3 of the License, or          *
* (at your option) any later version.                                        *
*                                                                            *
* This program is distributed in the hope that it will be useful,            *
* but WITHOUT ANY WARRANTY; without even the implied warranty of             *
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the               *
* GNU General Public License for more details.                               *
*                                                                            *
* You should have received a copy of the GNU General Public License          *
* along with this program. If not, see <http://www.gnu.org/licenses/>.       *
*                                                                            *
*  Author:        Vasco Ferraz                                               *
*  Contact:       http://vascoferraz.com/contact/                            *
*  Description:   http://vascoferraz.com/projects/ds18b20-data-logger/       *
*****************************************************************************/

// Import libraries
#include <Wire.h> // I2C and TWI library
#include <RTClib.h> // RTClib library
#include <OneWire.h> // OneWire library
#include <DallasTemperature.h> // DallasTemperature library
#include <SPI.h> // SPI library
#include <SD.h> // SD library

// Define constants
#define DS1307_I2C_ADDRESS 0x68 // Each I2C object has a unique bus address, the DS1307 (Real Time Clock) is 0x68
#define ONE_WIRE_BUS 2 // Define that the input/output data pin from the DS18B20 (temperature sensor) is connected in Arduino's digital I/O pin number 2
#define TEMPERATURE_PRECISION 12 // Define the precision of the conversion: 9bit, 10bit, 11bit or 12bit

// Declare objects
RTC_DS1307 RTC; // Declare variable RTC of type RTC_DS1307
OneWire oneWire(ONE_WIRE_BUS); // Setup a oneWire instance to communicate with any OneWire devices (not just Maxim/Dallas temperature ICs)
DallasTemperature sensors(&oneWire); // Pass our oneWire reference to Dallas Temperature. That is, tell Dallas Temperature Library to use oneWire Library
File myFile; // Declare variable myFile of type File

// Declare variables
DeviceAddress temperatureSensor1, temperatureSensor2, temperatureSensor3; // Arrays to store addresses
unsigned int second, minute, hour, dayOfTheWeek, day, month, year;
unsigned char previous_second=1; // This variable is used to write the datalog.txt at a pace of 10 seconds
unsigned int year_2000; // "year" goes from 0 up to 99 and not from 2000 up to 2099. This variable is used to simplify this situation
unsigned int counter=0; const unsigned int maxcounter=3000;
unsigned char flag=0;

byte decToBcd(byte val){return ( (val/10*16) + (val%10) );} // Convert normal decimal numbers to binary coded decimal
byte bcdToDec(byte val){return ( (val/16*10) + (val%16) );} // Convert binary coded decimal to normal decimal numbers

const char chipSelect = 10; // SD module's Chip Select (CS) pin
const char cardDetect = 3;  // SD module's Card Detect (CD) pin

// The setup function runs only once when you power or reset the board
void setup ()
{
  pinMode(cardDetect, INPUT); // Initialize the cardDetect pin as an input
  
  Serial.begin(9600); // Open serial port and sets data rate to 9600 bps
  Wire.begin(); // Start the Wire (I2C communications)
  RTC.begin(); // Start the RTC Chip
  sensors.begin(); // Start the DallasTemperature library
  
  while (! Serial); // Wait until Serial is ready (needed for Leonardo only)
  Serial.setTimeout(10); // Sets the maximum milliseconds to wait for serial data
  
  Serial.println(F("INITIALIZING:")); // Initialize services
  
  freeMem(); // Print free SRAM in Serial Monitor
  initSerialCommunication(); // Initialize serial communication
  initRTC(); // Initialize Real Time Clock
  initSDcard(); // Initialize SD card
  initOneWire(); // Initialize OneWire devices
  
  Serial.println();
  Serial.println(F("LOGGER IS RUNNING:"));
  repetitiveStrings();
}


// The loop function runs over and over again forever
void loop ()
{
  readClock(); // Read clock and store current time and date into the following variables: second, minute, hour, dayOfTheWeek, day, month and year
  
  if(Serial.available())
  {
   if (Serial.peek() == 'p')
   {
   Serial.read(); // Clear serial buffer
   Serial.println(F("- Printing current time, date and temperature -"));
   printTime(); // Print hour, minute and second in Serial Monitor
   Serial.print(F(" | "));
   printDate(); // Print day, month, year and weekday in Serial Monitor
   Serial.println();
   
   sensors.requestTemperatures();
   
   Serial.print(F("Device 0 Temperature: "));
   printTemperatureInC(temperatureSensor1); // Print the temperature of a device in ºC in Serial Monitor
   Serial.print(F("Device 1 Temperature: "));
   printTemperatureInC(temperatureSensor2); // Print the temperature of a device in ºC in Serial Monitor
   Serial.print(F("Device 2 Temperature: "));
   printTemperatureInC(temperatureSensor3); // Print the temperature of a device in ºC in Serial Monitor
   
   Serial.println();
   repetitiveStrings();
   }
   
   else if (Serial.peek() == 't')
   {
   Serial.read(); // Clear serial buffer
   Serial.println(F("- Entering time setup -"));
   Serial.print(F("Current time: "));
   printTime();
   Serial.println();
   
   setHour();
   setMinute();
   setSecond();
   
   Serial.print(F("Current time: "));
   printTime();
   Serial.println();
   Serial.println();
   repetitiveStrings();
   }
  
   else if (Serial.peek() == 'd')
   {
   Serial.read(); // Clear serial buffer
   Serial.println(F("- Entering date setup -"));
   Serial.print(F("Current date: "));
   printDate();
   Serial.println();
   
   setDay();
   setMonth();
   setYear();
   
   Serial.print(F("Current date: "));
   printDate();
   Serial.println();
   Serial.println();
   repetitiveStrings();
   }
  
   else if (Serial.peek() == 'u')
   {
   Serial.read(); // Clear serial buffer
   Serial.println(F("- SD card unmounting -"));
   Serial.print(F("SD card unmounted"));
   SD.end();
   Serial.println();
   Serial.println();
   repetitiveStrings();
   }
   
   else if (Serial.peek() == 'm')
   {
   Serial.read(); // Clear serial buffer
   Serial.println(F("- SD card mounting -"));
   Serial.print(F("SD card mounted"));
   SD.begin(chipSelect);
   Serial.println();
   Serial.println();
   repetitiveStrings();
   }
  
   else if (Serial.peek() != 'p' || Serial.peek() != 't' || Serial.peek() != 'd' || Serial.peek() != 'u' || Serial.peek() != 'm')
   {
   Serial.read(); // Clear serial buffer
   Serial.println(F("- Char not accepted -"));
   repetitiveStrings();
   }
  
   while (Serial.read() >= 0){;} // Flush remaining characters
  }
  
  if (digitalRead(cardDetect) == LOW && second%10 == 0 && previous_second != second)
   {
    temperatureLogger();
    previous_second = second;
   }
   
  if (digitalRead(cardDetect) == HIGH && second%10 == 0 && previous_second != second)
   {
    Serial.println(F("- Datalog status -"));
    Serial.println(F("Failed - SD card was removed"));
    Serial.println();
    SD.end();
    previous_second = second;
   }
}


// Print hour, minute and second in Serial Monitor
void printTime (void)
{
  DateTime now = RTC.now();
 
  // Print hour in Serial Monitor
  if (now.hour() <= 9)
  {
    Serial.print(F("0")); Serial.print(now.hour(), DEC); Serial.print(F(":"));
  }
    else { Serial.print(now.hour(), DEC); Serial.print(F(":")); }
 
  // Print minute in Serial Monitor
  if (now.minute() <= 9)
  {
    Serial.print(F("0")); Serial.print(now.minute(), DEC); Serial.print(F(":"));
  }
    else { Serial.print(now.minute(), DEC); Serial.print(F(":")); }
 
  // Print second in Serial Monitor
   if (now.second() <= 9)
  {
    Serial.print(F("0")); Serial.print(now.second(), DEC);
  }
    else { Serial.print(now.second(), DEC); }
}
 
 
// Print day, month, year and weekday in Serial Monitor
void printDate (void)
{
  DateTime now = RTC.now();
  
  // Print day in Serial Monitor
  if (now.day() <= 9)
  {
    Serial.print(F("0")); Serial.print(now.day(), DEC); Serial.print(F("/"));
  }
    else { Serial.print(now.day(), DEC); Serial.print(F("/")); }
 
  // Print month in Serial Monitor
  switch(now.month())
  {
  case 1:  Serial.print(F("Jan/")); break;
  case 2:  Serial.print(F("Feb/")); break;
  case 3:  Serial.print(F("Mar/")); break;
  case 4:  Serial.print(F("Apr/")); break;
  case 5:  Serial.print(F("May/")); break;
  case 6:  Serial.print(F("Jun/")); break;
  case 7:  Serial.print(F("Jul/")); break;
  case 8:  Serial.print(F("Aug/")); break;
  case 9:  Serial.print(F("Sep/")); break;
  case 10: Serial.print(F("Oct/")); break;
  case 11: Serial.print(F("Nov/")); break;
  case 12: Serial.print(F("Dec/")); break;
  default: Serial.print(F("Err/"));
  }
 
  // Print year in Serial Monitor
  Serial.print(now.year(), DEC);
 
  // Print weekday in Serial Monitor
  switch(now.dayOfTheWeek())
  {
  case 0:  Serial.print(F(" | Sunday"));    break;
  case 1:  Serial.print(F(" | Monday"));    break;
  case 2:  Serial.print(F(" | Tuesday"));   break;
  case 3:  Serial.print(F(" | Wednesday")); break;
  case 4:  Serial.print(F(" | Thursday"));  break;
  case 5:  Serial.print(F(" | Friday"));    break;
  case 6:  Serial.print(F(" | Saturday"));  break;
  default: Serial.print(F(" | Error"));
  }
}


// Read clock and store current time and date into the following variables: second, minute, hour, dayOfTheWeek, day, month and year
void readClock(void)
{
 Wire.beginTransmission(DS1307_I2C_ADDRESS);
 Wire.write(0x00);
 Wire.endTransmission();
 Wire.requestFrom(DS1307_I2C_ADDRESS, 7);
 second = bcdToDec(Wire.read());
 minute = bcdToDec(Wire.read());
 hour = bcdToDec(Wire.read());
 dayOfTheWeek = bcdToDec(Wire.read());
 day = bcdToDec(Wire.read());
 month = bcdToDec(Wire.read());
 year = bcdToDec(Wire.read());
}


// Function to adjust the RTC
void adjustClock (unsigned char address, unsigned int clockVariable)
{
 Wire.beginTransmission(DS1307_I2C_ADDRESS);
 Wire.write(address);
 Wire.write(decToBcd(clockVariable));
 Wire.endTransmission();
 Serial.println(F("Valid"));
 Serial.read(); // Clear serial buffer
 counter=maxcounter;
 flag=1;
}


// Set hour
void setHour (void)
{
 flag=0;
 counter=0;
 Serial.print(F("Enter hour [0,23]: "));
  
  if (Serial.available() == 0)
  {
   for (counter=0 ; counter <=maxcounter ; counter++)
   {
   delay(1);
   
   // Do not accept the entry if the introduced char is not a number (48 and 57 represents digit 0 and 9 in ASCII, respectively)
   if ( (Serial.peek() >= 0 && Serial.peek() <= 47) || (Serial.peek() >= 58 && Serial.peek() <= 255) )
    {
     Serial.read(); // Clear serial buffer
     Serial.println(F("Not valid"));
     setHour();
    }
     
   if ( Serial.peek() >= 48 && Serial.peek() <= 57 )
    {
     hour = Serial.parseInt();
     if (hour >= 0 && hour <= 23)
     {adjustClock (0x02, hour);}
     else {Serial.println(F("Not Valid")); Serial.read(); setHour();}   
    }
   }
    if (flag == 0)
     {
      flag=1;
      Serial.read(); // Clear serial buffer
      Serial.println(F("Unchanged"));
     }
  }
}


// Set minute
void setMinute (void)
{
 flag=0;
 counter=0;
 Serial.print(F("Enter minute [0,59]: "));
  
  if (Serial.available() == 0)
  {
   for (counter=0 ; counter <=maxcounter ; counter++)
   {
   delay(1);
   
   // Do not accept the entry if the introduced char is not a number (48 and 57 represents digit 0 and 9 in ASCII, respectively)
   if ( (Serial.peek() >= 0 && Serial.peek() <= 47) || (Serial.peek() >= 58 && Serial.peek() <= 255) )
    {
     Serial.read(); // Clear serial buffer
     Serial.println(F("Not valid"));
     setMinute();
    }
     
   if ( Serial.peek() >= 48 && Serial.peek() <= 57 )
    {
     minute = Serial.parseInt();
     if (minute >= 0 && minute <= 59)
     {adjustClock (0x01, minute);}
     else {Serial.println(F("Not Valid")); Serial.read(); setMinute();}
    }
   }
    if (flag == 0)
     {
      flag=1;
      Serial.read(); // Clear serial buffer
      Serial.println(F("Unchanged"));
     }
  }
}


// Set second
void setSecond (void)
{
 flag=0;
 counter=0;
 Serial.print(F("Enter second [0,59]: "));
  
  if (Serial.available() == 0)
  {
   for (counter=0 ; counter <=maxcounter ; counter++)
   {
   delay(1);
   
   // Do not accept the entry if the introduced char is not a number (48 and 57 represents digit 0 and 9 in ASCII, respectively)
   if ( (Serial.peek() >= 0 && Serial.peek() <= 47) || (Serial.peek() >= 58 && Serial.peek() <= 255) )
    {
     Serial.read(); // Clear serial buffer
     Serial.println(F("Not valid"));
     setSecond();
    }
     
   if ( Serial.peek() >= 48 && Serial.peek() <= 57 )
    {
     second = Serial.parseInt();
     if (second >= 0 && second <= 59)
     {adjustClock (0x00, second);}
     else {Serial.println(F("Not Valid")); Serial.read(); setSecond();}
    }
   }
    if (flag == 0)
     {
      flag=1;
      Serial.read(); // Clear serial buffer
      Serial.println(F("Unchanged"));
     }
  }
}


// Set day
void setDay (void)
{
 flag=0;
 counter=0;
 Serial.print(F("Enter day [1,31]: "));
  
  if (Serial.available() == 0)
  {
   for (counter=0 ; counter <=maxcounter ; counter++)
   {
   delay(1);
   
   // Do not accept the entry if the introduced char is not a number (48 and 57 represents digit 0 and 9 in ASCII, respectively)
   if ( (Serial.peek() >= 0 && Serial.peek() <= 47) || (Serial.peek() >= 58 && Serial.peek() <= 255) )
    {
     Serial.read(); // Clear serial buffer
     Serial.println(F("Not valid"));
     setDay();
    }
     
   if ( Serial.peek() >= 48 && Serial.peek() <= 57 )
    {
     day = Serial.parseInt();
     
     if (day >= 1 && day <= 31 && (month == 1 || month == 3 || month == 5 || month == 7 || month == 8 || month == 10 || month == 12))
     {adjustClock (0x04, day);}
     
     else if (day >= 1 && day <= 30 && (month == 4 || month == 6 || month == 9 || month == 11))
     {adjustClock (0x04, day);}
     
     else if (day >= 1 && day <= 29 && month == 2 && year%4 == 0)
     {adjustClock (0x04, day);}
     
     else if (day >= 1 && day <= 28 && month == 2 && year%4 != 0)
     {adjustClock (0x04, day);}   

     else {Serial.println(F("Not Valid")); Serial.read(); setDay();}
    }
   }
    if (flag == 0)
     {
      flag=1;
      Serial.read(); // Clear serial buffer
      Serial.println(F("Unchanged"));
     }
  }
}


// Set month
void setMonth (void)
{
 flag=0;
 counter=0;
 Serial.print(F("Enter month [1,12]: "));
  
  if (Serial.available() == 0)
  {
   for (counter=0 ; counter <=maxcounter ; counter++)
   {
   delay(1);
   
   // Do not accept the entry if the introduced char is not a number (48 and 57 represents digit 0 and 9 in ASCII, respectively)
   if ( (Serial.peek() >= 0 && Serial.peek() <= 47) || (Serial.peek() >= 58 && Serial.peek() <= 255) )
    {
     Serial.read(); // Clear serial buffer
     Serial.println(F("Not valid"));
     setMonth();
    }
     
   if ( Serial.peek() >= 48 && Serial.peek() <= 57 )
    {
     month = Serial.parseInt();
     
     if (month == 1 && day >= 1 && day <= 31 )
     {adjustClock (0x05, month);}
      
     else if (month == 2 && day >= 1 && day <= 29 && year%4 == 0)
     {adjustClock (0x05, month);}
      
     else if (month == 2 && day >= 1 && day <= 28 && year%4 != 0)
     {adjustClock (0x05, month);}
      
     else if (month == 3 && day >= 1 && day <= 31 )
     {adjustClock (0x05, month);}
      
     else if (month == 4 && day >= 1 && day <= 30 )
     {adjustClock (0x05, month);}
      
     else if (month == 5 && day >= 1 && day <= 31 )
     {adjustClock (0x05, month);}
      
     else if (month == 6 && day >= 1 && day <= 30 )
     {adjustClock (0x05, month);}
      
     else if (month == 7 && day >= 1 && day <= 31 )
     {adjustClock (0x05, month);}
      
     else if (month == 8 && day >= 1 && day <= 31 )
     {adjustClock (0x05, month);}
      
     else if (month == 9 && day >= 1 && day <= 30 )
     {adjustClock (0x05, month);}
      
     else if (month == 10 && day >= 1 && day <= 31 )
     {adjustClock (0x05, month);}
        
     else if (month == 11 && day >= 1 && day <= 30 )
     {adjustClock (0x05, month);}
      
     else if (month == 12 && day >= 1 && day <= 31 )
     {adjustClock (0x05, month);}

     else {Serial.println(F("Not Valid")); Serial.read(); setMonth();}
    }
   }
    if (flag == 0)
     {
      flag=1;
      Serial.read(); // Clear serial buffer
      Serial.println(F("Unchanged"));
     }
  }
}


// Set year
void setYear (void)
{
 flag=0;
 counter=0;
 Serial.print(F("Enter year [2014,2030]: "));
  
  if (Serial.available() == 0)
  {
   for (counter=0 ; counter <=maxcounter ; counter++)
   {
   delay(1);
   
   // Do not accept the entry if the introduced char is not a number (48 and 57 represents digit 0 and 9 in ASCII, respectively)
   if ( (Serial.peek() >= 0 && Serial.peek() <= 47) || (Serial.peek() >= 58 && Serial.peek() <= 255) )
    {
     Serial.read(); // Clear serial buffer
     Serial.println(F("Not valid"));
     setYear();
    }
     
   if ( Serial.peek() >= 48 && Serial.peek() <= 57 )
    {
     year_2000 = Serial.parseInt();         
     if (year_2000 >= 2014 && year_2000 <= 2030) 
     {year = year_2000 - 2000; adjustClock (0x06, year);}
     
     else {Serial.println(F("Not Valid")); Serial.read(); setYear();}
    }
   }
    if (flag == 0)
     {
      flag=1;
      Serial.read(); // Clear serial buffer
      Serial.println(F("Unchanged"));
     }
  }
}


// Repetitive strings
void repetitiveStrings(void)
{
 Serial.println(F("Type 'p' to print current time, date and temperature"));
 Serial.println(F("Type 't' or 'd' to setup the time or date"));
 Serial.println(F("Type 'm' or 'u' to mount or unmount the SD card"));
 Serial.println();
}


// Write time, date and temperature into the datalog file
void temperatureLogger(void)
{
 DateTime now = RTC.now();
 
 sensors.requestTemperatures();
 
 // Open the file. Keep in mind that only one file can be open at a time, so you have to close this one before opening another.
 myFile = SD.open("datalog.txt", FILE_WRITE); 
 
 if (!myFile)
 {
  Serial.println(F("- Datalog status -"));
  Serial.println(F("Failed - SD card was unmounted"));
  Serial.println();
 }
 
 else if (myFile)
 {
  // Write hour into the datalog file
  if (now.hour() <= 9)
  {
    myFile.print(F("0")); myFile.print(now.hour(), DEC); myFile.print(F(":"));
  }
    else { myFile.print(now.hour(), DEC); myFile.print(F(":")); }
 
  // Write minute into the datalog file
  if (now.minute() <= 9)
  {
    myFile.print(F("0")); myFile.print(now.minute(), DEC); myFile.print(F(":"));
  }
    else { myFile.print(now.minute(), DEC); myFile.print(F(":")); }
  
  // Write second into the datalog file
   if (now.second() <= 9)
  {
    myFile.print(F("0")); myFile.print(now.second(), DEC);
  }
    else { myFile.print(now.second(), DEC); }
       
  // Write day into the datalog file
  myFile.print(F(" | "));
  if (now.day() <= 9)
  {
    myFile.print(F("0")); myFile.print(now.day(), DEC); myFile.print(F("/"));
  }
    else { myFile.print(now.day(), DEC); myFile.print(F("/")); }
 
  // Write month into the datalog file
  switch(now.month())
  {
  case 1:  myFile.print(F("Jan/")); break;
  case 2:  myFile.print(F("Feb/")); break;
  case 3:  myFile.print(F("Mar/")); break;
  case 4:  myFile.print(F("Apr/")); break;
  case 5:  myFile.print(F("May/")); break;
  case 6:  myFile.print(F("Jun/")); break;
  case 7:  myFile.print(F("Jul/")); break;
  case 8:  myFile.print(F("Aug/")); break;
  case 9:  myFile.print(F("Sep/")); break;
  case 10: myFile.print(F("Oct/")); break;
  case 11: myFile.print(F("Nov/")); break;
  case 12: myFile.print(F("Dec/")); break;
  default: myFile.print(F("Err/"));
  }
 
  // Write year into the datalog file
  myFile.print(now.year(), DEC);
 
  // Write weekday into the datalog file
  switch(now.dayOfTheWeek())
  {
  case 0:  myFile.println(F(" | Sunday"));    break;
  case 1:  myFile.println(F(" | Monday"));    break;
  case 2:  myFile.println(F(" | Tuesday"));   break;
  case 3:  myFile.println(F(" | Wednesday")); break;
  case 4:  myFile.println(F(" | Thursday"));  break;
  case 5:  myFile.println(F(" | Friday"));    break;
  case 6:  myFile.println(F(" | Saturday"));  break;
  default: myFile.println(F(" | Error"));
  }
 
  // Write temperatures into the datalog file
  myFile.print(F("Device 0 Temperature: "));
  writeTemperatureInC(temperatureSensor1);
  myFile.print(F("Device 1 Temperature: "));
  writeTemperatureInC(temperatureSensor2);
  myFile.print(F("Device 2 Temperature: "));
  writeTemperatureInC(temperatureSensor3);
  
  myFile.println();
  myFile.close();
  Serial.println(F("- Datalog status -"));
  Serial.println(F("OK"));
  Serial.println();
 }
}


// Print the address of a device in Serial Monitor
void printAddress(DeviceAddress deviceAddress)
{
  for (uint8_t i = 0; i < 8; i++)
  {
    // Zero pad the address if necessary
    if (deviceAddress[i] < 16) Serial.print(F("0"));
    Serial.print(deviceAddress[i], HEX);
  }
}


// Print the resolution of a device in Serial Monitor
void printResolution(DeviceAddress deviceAddress)
{
  Serial.print(sensors.getResolution(deviceAddress), DEC);
  Serial.println(F(" bit"));
}


// Print the temperature of a device in ºC in Serial Monitor
void printTemperatureInC(DeviceAddress deviceAddress)
{
  float tempC = sensors.getTempC(deviceAddress);
  Serial.print(tempC);
  Serial.println(F(" degrees Celsius"));
}


// Write the temperature of a device in ºC into the SD memory card
void writeTemperatureInC(DeviceAddress deviceAddress)
{
  float tempC = sensors.getTempC(deviceAddress);
  myFile.print(tempC);
  myFile.println(F(" degrees Celsius"));
}


// Initialize serial communication
void initSerialCommunication()
{
 Serial.print(F("Serial communication: "));
  if (!Serial)
   {Serial.println(F("Failed")); delay(7000); initSerialCommunication();}
  else Serial.println(F("OK"));
}


// Initialize Real Time Clock
void initRTC()
{
 //RTC.adjust(DateTime(__DATE__, __TIME__)); // This line sets the RTC to the date and time this sketch was compiled
 Serial.print(F("RTC: "));
  if (!RTC.isrunning()) // If the clock is not running, execute the following code
   {
    Serial.print(F("Not Running! Restarting the RTC. "));
    // Starts ticking the clock
    Wire.beginTransmission(DS1307_I2C_ADDRESS);
    Wire.write(0x00); // Move pointer to 0x00 byte address
    Wire.write(0x00); // Sends 0x00. The whole byte is set to zero (0x00). This also means seconds will reset!! Unless you use a mask -> homework :)
    Wire.endTransmission();
    
    // Following line sets the RTC to the date & time to: 2015 January 01 - 00:00:00
    RTC.adjust(DateTime(2015,1,1, 0,0,0)); // Sequence: year, month, day,  hour, minute, second
   }
  if (RTC.isrunning())
   {Serial.println(F("OK"));}
  if (!RTC.isrunning())
   {Serial.println(F("Failed")); delay(7000); initRTC();}
}

// Initialize SD card
void initSDcard()
{
 Serial.print(F("SD card: "));
  pinMode(SS, OUTPUT); // On the Ethernet Shield, Chip Select (CS) is pin 4. It's set as an output by default.
                       // Note that even if it's not used as the CS pin, the hardware SS pin
                       // (10 on most Arduino boards, 53 on the Mega) must be left as an output or the SD library functions will not work.
  if (!SD.begin(chipSelect))
   {Serial.println(F("Failed")); delay(7000); initSDcard();}
  else Serial.println(F("OK"));
}


// Initialize OneWire devices
void initOneWire()
{
  Serial.println();
  Serial.println(F("DETECTING ONEWIRE DEVICES:")); // Detect all OneWire devices
  
  // Find all devices on the bus
  Serial.print(F("Devices found: "));
  Serial.println(sensors.getDeviceCount(), DEC);
   
  // Search for all devices on the bus and assign them based on an index.
  if (!sensors.getAddress(temperatureSensor1, 0)) Serial.println(F("Unable to find address for Device 0"));
  if (!sensors.getAddress(temperatureSensor2, 1)) Serial.println(F("Unable to find address for Device 1"));
  if (!sensors.getAddress(temperatureSensor3, 2)) Serial.println(F("Unable to find address for Device 2"));
  
  // Set the resolution of the temperature sensors
  sensors.setResolution(temperatureSensor1, TEMPERATURE_PRECISION);
  sensors.setResolution(temperatureSensor2, TEMPERATURE_PRECISION);
  sensors.setResolution(temperatureSensor3, TEMPERATURE_PRECISION);
  
  // Print the address and resolution of the temperature sensors found on the bus
  Serial.print(F("Device 0 Address/Resolution: "));
  printAddress(temperatureSensor1);
  Serial.print(F(" / "));
  printResolution(temperatureSensor1);
  
  Serial.print(F("Device 1 Address/Resolution: "));
  printAddress(temperatureSensor2);
  Serial.print(F(" / "));
  printResolution(temperatureSensor2);
 
  Serial.print(F("Device 2 Address/Resolution: "));
  printAddress(temperatureSensor3);
  Serial.print(F(" / "));
  printResolution(temperatureSensor3);
  
  // Is parasite power being used? For more information about parasite power you must read the datasheet of the DS18B20 temperature sensor
  Serial.print(F("Parasite power is: "));
  if (sensors.isParasitePowerMode()) Serial.println(F("ON"));
  else Serial.println(F("OFF"));
}


// Print free SRAM in Serial Monitor
uint16_t freeMem()
{
  char top;
  extern char *__brkval;
  extern char __bss_end;
  Serial.print(F("Available SRAM: "));
  Serial.print( __brkval ? &top - __brkval : &top - &__bss_end);
  Serial.println(F(" bytes"));
}
