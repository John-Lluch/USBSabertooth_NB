# USBSabertooth_NB

An Arduino NON BLOCKING library for Dimmension Engineering Sabertooth motor controllers.

# Motivation

The original library supplied by Dimmension Engineering BLOCKS program execution after sending a 'get' command, such as 'getCurrent', 'getBattery' and so on, until it gets response from the Sabertooth controller. In other words, the original library STOPS the execution of the program in a similar way that a delay() command, thus interfering with the smooth execution of the rest of the program. This may turn to be a serious problem on many Arduino projects where real time monitoring or timely control of devices is critical. Operations as simple as turning on and off a device on a precise interval may be compromised by the execution blocking of the SaberTooth library.

# Features

The USBSabertooth_NB library enable you to perform non-blocking write and reads to and from Sabertooth motor controllers. The library uses a similar approach to Serial non-blocking communications. The library is based on and very similar to the original one, with the addition of several class member functions for non-blocking communications.

# Version history

#### v 1.0.0   
Initial commit

#### v 1.1.0   
Minor bug fixes

#### v 1.1.1   
Improved error handling and recovery

#### v 1.1.2 
Code refactoring: all communications code is now moved to the USBSabertoothSerial class, the USBSabertooth class now remains a container for Sabertooth settings and bridges all communications to the USBSaberToothSerial instance.

# How it works

You implement non-blocking communications in a similar way than you would for serial communications. You send 'get' commands using one of the non-blocking 'async_get' functions. You check for a response to be available with one of the 'reply_available' functions. The 'reply_available' functions will only return true when a response is ready, and thus you can process it immediately without any delays. 

# USBSabertooth_NB Functions

List of new non-blocking 'get' functions on the USBSabertooth class:
```
boolean async_get(byte type, byte number, int context=0)
boolean async_getBattery(byte motorOutputNumber, int context=0, boolean unscaled = false)
boolean async_getCurrent(byte motorOutputNumber, int context=0, boolean unscaled = false)
boolean async_getTemperature(byte motorOutputNumber, int context=0, boolean unscaled=false )
```

List of new non-blocking 'available' functions on the USBSabertoothSerial:
```
boolean reply_available( byte *type, byte *number, USBSabertoothGetType *getType, int *result, int *context );
boolean reply_available( byte *type, byte *number, int *result, int *context );
boolean reply_available( byte *number, int *result, int *context );
boolean reply_available( int *result, int *context );
```

# Simple example

This example reads the Battery voltage, the Current of motors 1 and 2, and put their values in global variables in a non-blocking fashion. Execution time of the full loop is less than 1 ms in all cases, thus enabling you to perform whatever other tasks whitout any interfering execution delays

```
#include <USBSabertooth_NB.h>
USBSabertoothSerial STS; 
USBSabertooth ST(STS, 128);

int battery = 0;
int current1 = 0;
int current2 = 0;

void setup() 
{ 
   SabertoothTXPinSerial.begin(9600); 
   ST.async_getBattery( 1, 0 );  // start cycle by requesting battery voltage with context 0
}

void loop() 
{
  int result = 0;
  int context = 0;
  
  if ( STS.reply_available( &result, &context ) )
  {
      switch ( context )
      {
         case SABERTOOTH_GET_ERROR:      // Error or timeout
         case SABERTOOTH_GET_TIMED_OUT:  
           ST.async_getBattery( 1, 0 );  // reset the cycle by starting from the beginning
           break;
           
        case 0:  // Battery  
           battery = result;
           ST.async_getCurrent( 1, 1 );  // next request: get motor 1 current with context 1
           break;
           
        case 1: // Current 1
           current1 = result;
           ST.async_getCurrent( 2, 2 );  // next request: get motor 2 current with context 2
           break;

        case 2: // Current 2
           current2 = result;
           ST.async_getBattery( 1, 0 );  // next request: get battery voltage with context 0
           break;
      } 
  }

  // process battery, current1 and current2 values here
  // ...
}

```

# More

Find the 'NonBlockingRead" example in the Examples->Advanced folder, for a more complete implementation of a sequence of non-blocking reads and writes to a Sabertooth motor controller, with feedback on the Serial monitor. This example requires a Leonardo, Pro Micro or another arduino controller with dual serial port coms. 'Serial' is used for Serial monitor communications and 'Serial1' is used for Sabertooth communications.

