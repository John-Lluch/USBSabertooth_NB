
// Non Blocking Read Sample for USB Sabertooth Packet Serial
// This example performs multiple non-blocking reads from the Sabertooth. 
// Loop time is below 1 ms on a 16 MHz Leonardo or Pro Micro 
// This example assumes a board with Serial and Serial1 interfaces (only required for display purposes)

#include "USBSabertooth_NB.h"

USBSabertoothSerial C; // Use the Arduino TX pin. It connects to S1.
                       // See the SoftwareSerial example in 3.Advanced for how to use other pins.

USBSabertooth ST(C, 128); // The USB Sabertooth is on address 128 (unless you've changed it with DEScribe).
                          // We'll name its object ST.
                          //
                          // If you've set up your Sabertooth on a different address, of course change
                          // that here. For how to configure the Sabertooth, see the DIP Switch Wizard at
                          //   http://www.dimensionengineering.com/datasheets/USBSabertoothDIPWizard/start.htm
                          // Be sure to select Packet Serial Mode for use with this library.
                    
                        
int battery = 0;
int motor1 = 0;
int motor2 = 0;
int current1 = 0;
int current2 = 0;
int temperature1 = 0;
int temperature2 = 0;

void setup() 
{
    // initialize serial communication at 9600 bits per second:
    Serial.begin(9600);
  
    SabertoothTXPinSerial.begin(9600); // 9600 is the default baud rate for Sabertooth Packet Serial.
                                     // You can change this with the DEScribe software, available at
                                     //   http://www.dimensionengineering.com/describe

   // start reading from sabertooth
   bool done = ST.async_getBattery( 1, 0 );  // request battery voltage with context 0

   Serial.println( "Serial TEST" );
}

void loop() 
{
  int result = 0;
  int context = 0;
  
  if ( ST.reply_available( &result, &context ) )
  {
    bool done = false; 
    if ( result == SABERTOOTH_GET_ERROR || result == SABERTOOTH_GET_TIMED_OUT )
    {
       Serial.print( "ERROR " );
       Serial.print( millis() );
       Serial.print( " *** ");
       Serial.println( result );
       done = ST.async_getBattery( 1, 0 );  // try again from the beginning
    }
    else 
    {
      switch ( context )
      {
        case 0:  // get Battery  
           battery = result;
           ST.async_get( 'M', 1, 1 );   // request motor 1 value voltage with context 1
           break;
           
        case 1: // get value
           motor1 = result;
           done = ST.async_get( 'M', 2, 2 );  // request motor 2 value voltage with context 2
           break;

        case 2: // get value
           motor2 = result;
           done = ST.async_getCurrent( 1, 3 );  // request motor 1 current with context 3
           break;
           
        case 3: // get current
           current1 = result;
           done = ST.async_getCurrent( 2, 4 );  // request motor 2 current with context 4
           break;

        case 4: // get current
           current2 = result;
           done = ST.async_getTemperature( 1, 5 );  // request temperature 1 with context 5
           break;

        case 5: // get temperature
           temperature1 = result;
           done = ST.async_getTemperature( 2, 6 );  // request temperature 2 voltage with context 6
           break;
           
        case 6: // get temperature
           temperature2 = result;
           done = ST.async_getBattery( 1, 0 );  // request battery voltage with context 0
           break; 
      } 
    }
  }

// the following code sets an arbitraries values to motor 1 and 2 every second

    unsigned long current = millis();
    static long start = 0;
    static byte motor = 1;
    static bool state = 0;
    static bool dir = 0;
    if ( current-start > 1000 )
    {
        start = current;
        
        state = !state;
        if ( state ) dir = !dir;
        if ( state && dir ) motor = (motor==1?2:1);

        if ( state )
        {
          ST.freewheel(motor, false); // Turn off freewheeling.
          ST.motor(motor, dir?1000:-1000);
        }
        else
        {
          ST.freewheel(motor, true); // Turn off freewheeling.
          ST.motor(motor, 0);
        }
    }

// the following code is just for display purposes, only values that changed are displayed

  static int dsp_battery = -1;
  if (dsp_battery != battery )
  {
    dsp_battery = battery;
    char t[12];
    sprintf( t, "Batt:%3d", battery );
    Serial.println( t );
  }

  static int dsp_motor1 = -1;
  if (dsp_motor1 != motor1 )
  {
    dsp_motor1 = motor1;
    char t[12];
    sprintf( t, "Mot1:%5d", motor1 );
    Serial.println( t );
  }

  static int dsp_motor2 = -1;
  if (dsp_motor2 != motor2 )
  {
    dsp_motor2 = motor2;
    char t[12];
    sprintf( t, "Mot2:%5d", motor2 );
    Serial.println( t );
  }

  static int dsp_current1 = -1;
  if (dsp_current1 != current1 )
  {
    dsp_current1 = current1;
    char t[12];
    sprintf( t, "Cur1:%5d", current1 );
    Serial.println( t );
  }

  static int dsp_current2 = -1;
  if (dsp_current2 != current2 )
  {
    dsp_current2 = current2;
    char t[12];
    sprintf( t, "Cur2:%5d", dsp_current2 );
    Serial.println( t );
  }

  static int dsp_temperature1 = -1;
  static int dsp_temperature2 = -1;
  if (dsp_temperature1 != temperature1 || dsp_temperature2 != temperature2 )
  {
    dsp_temperature1 = temperature1;
    dsp_temperature2 = temperature2;
    char t[12];
    sprintf( t, "Temp:%2d/%2d", dsp_temperature1, dsp_temperature2 );
    Serial.println( t );
  }

 // uncomment the following code to verify that the entire loop is executed in less than 1 millisecond, 
 // the delay of 1 ms is only added to help the console to catch up with the loop speed!
 
/*
  Serial.print( "loop speed test, milliseconds: " );
  Serial.print( millis() );
  Serial.print( " motor1: " );
  Serial.print( motor1 );
  Serial.print( " motor2: " );
  Serial.println( motor2 );
  delay(1);
*/
}
