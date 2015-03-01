// Erik de Ruiter - The Netherlands
// 2015-03-01
//
//---------------------------------------------------------------------------------------------------------------------
// Libraries
//---------------------------------------------------------------------------------------------------------------------
#include <SPI.h>                            // http://arduino.cc/en/Reference/SPI
#include <Streaming.h>                      // http://arduiniana.org/libraries/streaming/
/*
  This library works for any class that derives from Print:
    Serial << "Counter: " << counter;
    lcd << "Temp: " << t.get_temperature() << " degrees";
    my_pstring << "Hi Mom!" << endl;
  With the library you can also use formatting manipulators like this:
    Serial << "Byte value: " << _HEX(b) << endl;
    lcd << "The key pressed was " << _BYTE(c) << endl;
*/
#include <MAX7221spi.h>                     // https://code.google.com/p/max7221float/          
/*
    initialize(unsigned char no_of_digits)         Initilizes the MAX7221 and SPI library for normal operation.
    Initialize the number of digits used.
   
    called for normal operation:
    shutdown(OFF)
    testDisplay(NORMAL)
    setBrightness(15)
    
    The following methods can be called independently from initialize():
    
    setBrightness(unsigned char brightness)        Any value between 1 and 15, default is 15.
    shutdown(unsigned char choice)                 Two choices ON or OFF, default is OFF. 
                                                   The method shuts down the display but you can still write to the MAX7221.
                                                   This is an internal funtion of the MAX7221.
                                                   testDisplay() will overwrite the shutdown() function.
    
    testDisplay(unsigned char choice)              Two choices TEST or NORMAL, default is NORMAL.
                                                   All digits are turned on at full brightness.
                                                   This is an internal funtion of the MAX7219/7221.
    
    print(float fnum, unsigned char precision)     Displays the float number with the required precision.
                                                   The decimal point will vary according to the precision value.
                                                   Leading zero blanking always follows the digit after 
                                                   the decimal point for positive numbers and after the 
                                                   negative sign for negative numbers.  
*/
MAX7221spi myDisplay;                       // create a SPI Maxim 7219 or 7221 library instance  
//MAX7219/7221 -> Arduino:
//   DIN       -> 11 = MOSI    (Arduino output)
//   CLK       -> 13 = SCK     (Arduino output)
//   LOAD/#CS  -> 10 = SS      (Arduino output)

#define DATARECEIVEDPIN 7                   // LED pin. Each time a complete weather data value is received, the LED is lit

#define SW0 A0                              // define DIP switches input ports, used to set the desired weather data to display
#define SW1 A1
#define SW2 A2
#define SW3 A3
#define SW4 A4
#define SW5 A5

// >>>>>>>>  PIN 1 = Arduino serial RX port = TTL-RS232 input from Peet Bros Ultimeter

// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
// !!!!!!!  DON'T FORGET TO DISCONNECT PIN 2 (RS232 INPUT) WHEN UPLOADING NEW SKETCH  !!!!!!!
// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!


//---------------------------------------------------------------------------------------------------------------------
// definitions
//---------------------------------------------------------------------------------------------------------------------

int serialData      = 0;                    // holds one byte the incoming serial stream
int byte0           = 0;                    // byte0 to byte3 contain the ASCII characters from the serial stream
int byte1           = 0;                    // 1 weather data value (WIND SPEED, TEMP etc) consist of 4 bytes
int byte2           = 0;                    // so we have to collect all 4 bytes and then process them later.
int byte3           = 0;                    // 
int hexValue        = 0;                    // contains the HEX value of the serial stream ASCII characters
int weatherData     = 0;                    // contains the DECIMAL -ready to display- weather data
int positionCounter = 0;                    // counter to determine at which position serial stream is read
boolean validData   = false;                // used to check if all 4 bytes are available for display

int swBit0          = 0;                    // 6 DIP switches to set the desired weather values to display
int swBit1          = 0;                    
int swBit2          = 0;                    
int swBit3          = 0;
int swBit4          = 0;
int swBit5          = 0;
int swVal           = 0;                    // contains the decimal value of the DIP switches


//=====================================================================================================================
// SETUP
//=====================================================================================================================
void setup()
{
  Serial.begin (2400);                      // start serial for receiving data from PEET BROS WEATHER STATION
  pinMode(DATARECEIVEDPIN, OUTPUT);         // LED indication for received weatherData
  
  pinMode(SW0, INPUT);                      // 5 DIP switches, used to determine which value to display
  pinMode(SW1, INPUT);
  pinMode(SW2, INPUT);
  pinMode(SW3, INPUT);
  pinMode(SW4, INPUT);
  pinMode(SW5, INPUT);
  
  myDisplay.initialize(4);                  // number of digits used
  myDisplay.testDisplay(TEST);              // turn all the digits on
  delay(1000);
  myDisplay.testDisplay(NORMAL);            // back to normal

  checkSwitches();                          // check and display the DIP SWITCH setting
  myDisplay.print(swVal,0);                 // so the user can see if the right weather data to display is selected
  delay(2000);

}  // end of setup


//=====================================================================================================================
// Loop()
//=====================================================================================================================
void loop()
{
  checkSwitches();                          // check DIP switch setting so that the desired weather data is displayed
  processIncomingData ();                   // read incoming serial data and display it according to DIP switch setting
}







//=====================================================================================================================
// processIncomingByte (called from LOOP)
//=====================================================================================================================
void processIncomingData()
{
  if (Serial.available() > 0 )
  {
    digitalWrite(DATARECEIVEDPIN, LOW);                                   // reset LED indicator
    serialData = Serial.read();                                           // read each byte as DECIMAL <ASCII> value
    if (serialData == '\r')                                               // look for end (CR) of the Serial message
    {                     
      positionCounter = 0;                                                // beginning of Serial message detected, reset position counter
      validData = 0;                                                      // no valid data yet...

    }                       
    else                      
    {                     
      switch (swVal)                                                      // process weather data according to dip switch value
      {                     
          /////////////////////////////////////////////////////////////////////////////////////////////////////////////  
          case 3:                                                         // DIP SWITCH value 1: WIND SPEED KM/HOUR
            switch (positionCounter)
            {
              // once ever 1.9 seconds, the Weather Station puts out 115 different weather data values!
              // only WIND SPEED and WIND DIRECTION data appeares 3 times -evenly spaced- 
              // in the serial data stream to get a higher refresh rate for these items.
              //
              // capture 1st instance of weather data data from serial stream
              case 5:                                                     // get 1st MSByte of 4 byte value at position 5 in serial stream
                byte0 = 4096 * ascii2hex(serialData);                     // convert incoming ASCII char to HEX and compute DEC value
                break;                                                    // break out and look for second byte
              case 6:                                                     // get 2nd byte
                byte1 = 256 * ascii2hex(serialData);
                break;
              case 7:                                                     // get 3rd byte
                byte2 = 16 * ascii2hex(serialData);
                break;
              case 8:
                byte3 = ascii2hex(serialData);                            // get 4th and LSByte, we are ready for conversion now!
                weatherData = (byte0 + byte1 + byte2 + byte3);            // add all DEC byes values together
                validData = 1;                                            // four bytes captured in so ready to display
                digitalWrite(DATARECEIVEDPIN, HIGH);
                break;
              // capture 2nd instance of weather data data from serial stream
              case 137:
                byte0 = 4096 * ascii2hex(serialData);
                break;
              case 138:
                byte1 = 256 * ascii2hex(serialData);
                break;
              case 139:
                byte2 = 16 * ascii2hex(serialData);
                break;
              case 140:
                byte3 = ascii2hex(serialData);
                weatherData = (byte0 + byte1 + byte2 + byte3);
                validData = 1; 
                digitalWrite(DATARECEIVEDPIN, HIGH);
                break;
              // capture 3rd instance of weather data data from serial stream
              case 285:
                byte0 = 4096 * ascii2hex(serialData);
                break;
              case 286:
                byte1 = 256 * ascii2hex(serialData);
                break;
              case 287:
                byte2 = 16 * ascii2hex(serialData);
                break;
              case 288:
                byte3 = ascii2hex(serialData);
                weatherData = (byte0 + byte1 + byte2 + byte3);
                validData = 1; 
                digitalWrite(DATARECEIVEDPIN, HIGH);
                break;
            } // switch
            positionCounter++;                                            // increase counter and repeat
            // DISPLAY DATA            
            if (validData == 1)                                           // display data only if data is present/changed
            {
              myDisplay.print((float)weatherData/10,1);                   // print the received weather data, divided/10
            }
            validData = 0;                                                // reset 'display data' flag
            break;
          /////////////////////////////////////////////////////////////////////////////////////////////////////////////  
          case 4:                                                         // DIP SWITCH value 1: WIND SPEED BEAFORT
            // do something
            break;
          /////////////////////////////////////////////////////////////////////////////////////////////////////////////  
          case 5:                                                         // DIP SWITCH value 1: WIND SPEED KM/H
            // do something
            break;
          /////////////////////////////////////////////////////////////////////////////////////////////////////////////  
          case 6:                                                         // DIP SWITCH value 1: WIND SPEED M/S
            // do something
            break;
      } // switch(swVal)
    } // else
  } // if (Serial.available() > 0 )
} // void processIncomingByte ()

//=====================================================================================================================
// checkSwitches (called from SETUP and LOOP)
//=====================================================================================================================
void checkSwitches()
{
  swBit0 = digitalRead(SW0) *  1;           // LSB    get switch positions and convert BIN to DEC
  swBit1 = digitalRead(SW1) *  2;
  swBit2 = digitalRead(SW2) *  4;
  swBit3 = digitalRead(SW3) *  8;
  swBit4 = digitalRead(SW4) * 16;
  swBit5 = digitalRead(SW5) * 32;           // MSB
  swVal = swBit0 + swBit1 + swBit2 + swBit3 + swBit4 + swBit5;
}

//================================================================================================================
// ASCII to INT (calles from processIncomingData)
//================================================================================================================
int ascii2hex(int asciiValue)
{
  if (asciiValue >= 48 && asciiValue <= 57)  
    hexValue = asciiValue - 48;
  else if (asciiValue >= 65 && asciiValue <= 70)
    hexValue = asciiValue - 55;
  return hexValue;
}

/*================================================================================================================
 // c o m m e n t s
 //================================================================================================================
 
 This is one complete line of data coming in:
 
 - header (&CR&)
 - 452 HEX digits
 - CR and LF
 (the dashes mean no data available)
 
 EXAMPLE:
 0         1         2         3         4         5         6         7         8         9        9
 0123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789
 |...|...|...|...|...|...|...|...|...|...|...|...|...|...|...|...|...|...|...|...|...|...|...|...|...
 &CR&007000CC00F400BB029402940000273E000186780001034D------------00E004BE023901680241017200E002390168
 023901680241017200E002390168273A0326007000CC275304CB00DD26F1042A02E201B402EA01B700E002E201B403F20000
 03F2000000DC0000034603F2000003F2000000DC00000346013C041E01CD033100DD01CE050402DD0380006F00D602CC0387
 00DC02FF03BE275C00002774026500DC2823034D034E04AC033904A700DC0356034F000000000000000000DC000003460000
 00000000000000DC00000346000200DD001600020000BEB8B5000071
 
 for example (above data string):
 The first 4 bytes = 1. Wind Speed (0.1kph) = 0070h = 112 DEC = 11,2 km/h
 The 2nd 4 bytes = 2. Current Wind Direction (0-255) = 00CCh = 204 degrees
 etc.
 
 
 RECORD STRUCTURE:
 
 Header = &CR&, Data FIELDs (note:  FIELD nos. 111-114 are 2-digits; all others are 4-digits)
 
 
 VALUES FOR WIND SPEED AND DIRECTION ARE REPEATED 3 TIMES IN THE STREAM TO GET MORE RESPONSIVE DISPLAY
 
 
 Field  byte   byte  Description
        Start  End
   x      0          LF
   x      1          &
   x      2          C
   x      3          R
   x      4          &
   1.     5     8 *  Wind Speed (0.1kph)
   2.     9    12 *  Current Wind Direction (0-255)
   3.    13    16    5 minute Wind Speed Peak (0.1 kph)
   4.    17    20    5 minute Wind Direction Peak (0-255)
   5.    21    24    Wind Chill (0.1 deg F)
   6.    25    28    Outdoor Temp (0.1deg F)  (’97 and later)
   7.    29    32    Rain Total for today (0.01 in.)
   8.    33    36    Barometer (0.1 mbar)
   9.    37    40    Barometer 3-Hour Pressure Change (0.1 mbar)
  10.    41    44    Barometer Correction Factor(LSW)
  11.    45    48    Barometer Correction Factor(MSW)
  12.    49    52    Indoor Temp (0.1 deg F)
  13.    53    56    Outdoor Humidity (.1%)
  14.    57    60    Indoor Humidity (.1%)
  15.    61    64    Dew Point (0.1 deg F)
  16.    65    68    Date (day of year)
  17.    69    72    Time (minute of day)
  18.    73    76    Today's Low Chill Date
  19.    77    80    Today's Low Chill Time
  20.    81    84    Yesterday's Low Chill Date
  21.    85    88    Yesterday's Low Chill Time
  22.    89    92    Long Term Low Chill Date
  23.    93    96    Long Term Low Chill Date
  24.    97   100    Long Term Low Chill Time
  25.   101   104    Today's Low Outdoor Temp Date
  26.   105   108    Today's Low Outdoor Temp Time
  27.   109   112    Yesterday's Low Outdoor Temp Date
  28.   113   116    Yesterday's Low Outdoor Temp Time
  29.   117   120    Long Term Low Outdoor Temp Date
  30.   121   124    Long Term Low Outdoor Temp Date
  31.   125   128    Long Term Low Outdoor Temp Time
  32.   129   132    Today's Low Barometer Date
  33.   133   136    Today's Low Barometer Time
  34.   137   140 *  Wind Speed (0.1kph)
  35.   141   144 *  Current Wind Direction (0-255)
  36.   145   148    Yesterday's Low Barometer Date
  37.   149   152    Yesterday's Low Barometer Time
  38.   153   156    Long Term Low Barometer Date
  39.   157   160    Long Term Low Barometer Date
  40.   161   164    Long Term Low Barometer Time
  41.   165   168    Today's Low Indoor Temp Date
  42.   169   172    Today's Low Indoor Temp Time
  43.   173   176    Yesterday's Low Indoor Temp Date
  44.   177   180    Yesterday's Low Indoor Temp Time
  45.   181   184    Long Term Low Indoor Temp Date
  46.   185   188    Long Term Low Indoor Temp Date
  47.   189   192    Long Term Low Indoor Temp Time
  48.   193   196    Today's Low Outdoor Humidity Date
  49.   197   200    Today's Low Outdoor Humidity Time
  50.   201   204    Yesterday's Low Outdoor Humidity Date
  51.   205   208    Yesterday's Low Outdoor Humidity Time
  52.   209   212    Long Term Low Outdoor Humidity Date
  53.   213   216    Long Term Low Outdoor Humidity Date
  54.   217   220    Long Term Low Outdoor Humidity Time
  55.   221   224    Today's Low Indoor Humidity Date
  56.   225   228    Today's Low Indoor Humidity Time
  57.   229   232    Yesterday's Low Indoor Humidity Date
  58.   233   236    Yesterday's Low Indoor Humidity Time
  59.   237   240    Long Term Low Indoor Humidity Date
  60.   241   244    Long Term Low Indoor Humidity Date
  61.   245   248    Long Term Low Indoor Humidity Time
  62.   249   252    Today's Wind Speed Date
  63.   253   256    Today's Wind Speed Time
  64.   257   260    Yesterday's Wind Speed Date
  65.   261   264    Yesterday's Wind Speed Time
  66.   265   268    Long Term Wind Speed Date
  67.   269   272    Long Term Wind Speed Date
  68.   273   276    Long Term Wind Speed Time
  69.   277   280    Today's High Outdoor Temp Date
  70.   281   284    Today's High Outdoor Temp Time
  71.   285   288 *  Wind Speed (0.1kph)
  72.   289   292 *  Current Wind Direction (0-255)
  73.   293   296    Yesterday's High Outdoor Temp Date
  74.   297   300    Yesterday's High Outdoor Temp Time
  75.   301   304    Long Term High Outdoor Temp Date
  76.   305   308    Long Term High Outdoor Temp Date
  77.   309   312    Long Term High Outdoor Temp Time
  78.   313   316    Today's High Barometer Date
  79.   317   320    Today's High Barometer Time
  80.   321   324    Yesterday's High Barometer Date
  81.   325   328    Yesterday's High Barometer Time
  82.   329   332    Long Term High Barometer Date
  83.   333   336    Long Term High Barometer Date
  84.   337   340    Long Term High Barometer Time
  85.   341   344    Today's High Indoor Temp Date
  86.   345   348    Today's High Indoor Temp Time
  87.   349   352    Yesterday's High Indoor Temp Date
  88.   353   356    Yesterday's High Indoor Temp Time
  89.   357   360    Long Term High Indoor Temp Date
  90.   361   364    Long Term High Indoor Temp Date
  91.   365   368    Long Term High Indoor Temp Time
  92.   369   372    Today's High Outdoor Humidity Date
  93.   373   376    Today's High Outdoor Humidity Time
  94.   377   380    Yesterday's High Outdoor Humidity Date
  95.   381   384    Yesterday's High Outdoor Humidity Time
  96.   385   388    Long Term High Outdoor Humidity Date
  97.   389   392    Long Term High Outdoor Humidity Date
  98.   393   396    Long Term High Outdoor Humidity Time
  99.   397   400    Today's High Indoor Humidity Date
 100.   401   404    Today's High Indoor Humidity Time
 101.   405   408    Yesterday's High Indoor Humidity Date
 102.   409   412    Yesterday's High Indoor Humidity Time
 103.   413   416    Long Term High Indoor Humidity Date
 104.   417   420    Long Term High Indoor Humidity Date
 105.   421   424    Long Term High Indoor Humidity Time
 106.   425   428    Yesterday's Rain Total (0.01")
 107.   429   432    Long Term Rain Date
 108.   433   436    Long Term Rain Total (0.01")
 109.   437   440    Leap Year Date (0-3)
 110.   441   444    WDCF weatherData (0-255)
 111.   445   446    Today's High Wind Direction (2 digits)
 112.   447   448    Yesterday's High Wind Direction (2 digits)
 113.   449   450    Long Term High Wind Direction (2 digits)
 114.   451   451    Spare (2 digits)
 115.   453   456    1 Minute Wind Speed Average (0.1kph)
 x      457          Carriage Return
 
 
  Total size: 452 characters (hex digits) plus header, carriage return and line feed.
 
 

===========================================================================================================================================================
Peet Bros Weather Picture DIP SWITCH settings

SWITCH CHECK
123456 VALUE FUNCTION & UNITS
------ ----- ----------------------------------------------------------
↓↑↑↑↑↑  1    CURRENT TIME AND DATE
↑↓↑↑↑↑  2    CURRENT WIND SPEED (mph)
↓↓↑↑↑↑  3    CURRENT WIND SPEED (kph)
↑↑↓↑↑↑  4    CURRENT WIND SPEED (m/s)
↓↑↓↑↑↑  5    CURRENT WIND SPEED (knots)
↑↓↓↑↑↑  6    CURRENT WIND SPEED (Beaufort)
↓↓↓↑↑↑  7    1 MINUTE WIND SPEED AVERAGE (mph)
↑↑↑↓↑↑  8    1 MINUTE WIND SPEED AVERAGE (kph)
↓↑↑↓↑↑  9    1 MINUTE WIND SPEED AVERAGE (m/s)
↑↓↑↓↑↑ 10    1 MINUTE WIND SPEED AVERAGE (knots)
↓↓↑↓↑↑ 11    PEAK SPEED OVER LAST 5 MINUTES (mph)
↑↑↓↓↑↑ 12    PEAK SPEED OVER LAST 5 MINUTES (kph)
↓↑↓↓↑↑ 13    PEAK SPEED OVER LAST 5 MINUTES (m/s)
↑↓↓↓↑↑ 14    PEAK SPEED OVER LAST 5 MINUTES (knots)
↓↓↓↓↑↑ 15    TODAY'S HIGH WIND SPEED (mph)      WITH TIME OCCURRED
↑↑↑↑↓↑ 16    TODAY'S HIGH WIND SPEED (kph)      WITH TIME OCCURRED
↓↑↑↑↓↑ 17    TODAY'S HIGH WIND SPEED (m/s)      WITH TIME OCCURRED
↑↓↑↑↓↑ 18    TODAY'S HIGH WIND SPEED (knots)    WITH TIME OCCURRED

↑↑↓↓↓↓ 60    WIND DIRECTION IN DEGREES (0°=NORTH)

↓↓↑↑↓↑ 19    CURRENT WIND CHILL TEMPERATURE (°F)
↑↑↓↑↓↑ 20    CURRENT WIND CHILL TEMPERATURE (°C)
↓↑↓↑↓↑ 21    TODAY'S LOW WIND CHILL TEMP (°F)   WITH TIME OCCURRED
↑↓↓↑↓↑ 22    TODAY'S LOW WIND CHILL TEMP (°C)   WITH TIME OCCURRED
↓↓↓↑↓↑ 23    CURRENT OUTDOOR TEMPERATURE (°F)
↑↑↑↓↓↑ 24    CURRENT OUTDOOR TEMPERATURE (°C)
↓↑↑↓↓↑ 25    TODAY'S LOW OUTDOOR TEMP (°F)      WITH TIME OCCURRED
↑↓↑↓↓↑ 26    TODAY'S LOW OUTDOOR TEMP (°C)      WITH TIME OCCURRED
↓↓↑↓↓↑ 27    TODAY'S HIGH OUTDOOR TEMP (°F)     WITH TIME OCCURRED
↑↑↓↓↓↑ 28    TODAY'S HIGH OUTDOOR TEMP (°C)     WITH TIME OCCURRED

↓↓↑↑↑↓ 35    CURRENT OUTDOOR HUMIDITY (%)
↑↓↓↑↑↓ 38    TODAY'S LOW OUTDOOR HUMIDITY (%)   WITH TIME OCCURRED
↓↓↓↑↑↓ 39    TODAY'S HIGH OUTDOOR HUMIDITY (%)  WITH TIME OCCURRED

↓↑↓↓↓↑ 29    CURRENT BAROMETER (inHg)
↑↓↓↓↓↑ 30    CURRENT BAROMETER (mmHg)
↓↓↓↓↓↑ 31    CURRENT BAROMETER (mBar)
↓↓↓↓↓↑ 31    CURRENT BAROMETER (Hpa)
↑↑↑↑↑↓ 32    3 HOUR PRESSURE CHANGE (inHg)
↓↑↑↑↑↓ 33    3 HOUR PRESSURE CHANGE (mmHg)
↑↓↑↑↑↓ 34    3 HOUR PRESSURE CHANGE (mBar)
↑↓↑↑↑↓ 34    3 HOUR PRESSURE CHANGE (Hpa)

↑↑↓↑↑↓ 36    CURRENT DEW POINT TEMPERATURE (°F)
↓↑↓↑↑↓ 37    CURRENT DEW POINT TEMPERATURE (°C)

↑↓↑↓↑↓ 42    TODAY'S RAIN TOTAL (inches)
↓↓↑↓↑↓ 43    TODAY'S RAIN TOTAL (mm)
↑↑↓↓↑↓ 44    YESTERDAY'S RAIN TOTAL (inches) & DATE
↓↑↓↓↑↓ 45    YESTERDAY'S RAIN TOTAL (mm) & DATE
↑↓↓↓↑↓ 46    LONG TERM RAIN TOTAL (inches) SINCE DATE
↓↓↓↓↑↓ 47    LONG TERM RAIN TOTAL (mm) SINCE DATE

↑↑↑↓↑↓ 40    LONG TERM EVAPOTRANSPIRATION (0.01inches)
↓↑↑↓↑↓ 41    LONG TERM EVAPOTRANSPIRATION (0.1mm)

↑↑↑↑↓↓ 48    CURRENT INDOOR TEMPERATURE (°F)
↓↑↑↑↓↓ 49    CURRENT INDOOR TEMPERATURE (°C)
↑↓↑↑↓↓ 50    TODAY'S LOW INDOOR TEMP (°F)       WITH TIME OCCURRED
↓↓↑↑↓↓ 51    TODAY'S LOW INDOOR TEMP (°C)       WITH TIME OCCURRED
↑↑↓↑↓↓ 52    TODAY'S HIGH INDOOR TEMP (°F)      WITH TIME OCCURRED
↓↑↓↑↓↓ 53    TODAY'S HIGH INDOOR TEMP (°C)      WITH TIME OCCURRED
↑↓↓↑↓↓ 54    CURRENT INDOOR HUMIDITY (%)
↓↓↓↑↓↓ 55    TODAY'S LOW INDOOR HUMIDITY (%)    WITH TIME OCCURRED
↑↑↑↓↓↓ 56    TODAY'S HIGH INDOOR HUMIDITY (%)   WITH TIME OCCURRED

↓↑↑↓↓↓ 57    CURRENT HEAT INDEX (°F)
↑↓↑↓↓↓ 58    CURRENT HEAT INDEX (°C)

↑↑↑↑↑↑  0    SOLAR RADIATION (W/m2)
↓↓↑↓↓↓ 59    X-SENSOR (%)
↓↑↓↓↓↓ 61    UV RADIATION (W/m2)
↑↓↓↓↓↓ 62    UV INDEX


 ===========================================================================================================================================================
 
 ===========================================================================================================================================================
 
 */




