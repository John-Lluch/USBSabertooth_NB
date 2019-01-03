/*
Arduino Library for USB Sabertooth Packet Serial
Copyright (c) 2013 Dimension Engineering LLC
http://www.dimensionengineering.com/arduino

Permission to use, copy, modify, and/or distribute this software for any
purpose with or without fee is hereby granted, provided that the above
copyright notice and this permission notice appear in all copies.

THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER
RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT,
NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE
USE OR PERFORMANCE OF THIS SOFTWARE.
*/

#ifndef USBSabertooth_NB_h
#define USBSabertooth_NB_h   

/*!
\file USBSabertooth_NB.h
Include this file to use the USB Sabertooth Arduino library.
*/

#if defined(ARDUINO) && ARDUINO < 100
#error "This library requires Arduino 1.0 or newer."
#endif

#include <Arduino.h>

#if defined(USBCON)
#define SabertoothTXPinSerial Serial1 // Arduino Leonardo has TX->1 on Serial1, not Serial.
#else
#define SabertoothTXPinSerial Serial
#endif
#define SyRenTXPinSerial SabertoothTXPinSerial

#define SABERTOOTH_COMMAND_MAX_BUFFER_LENGTH    10
#define SABERTOOTH_COMMAND_MAX_DATA_LENGTH      5
#define SABERTOOTH_GETCOMMAND_DATA_LENGTH       3
#define SABERTOOTH_DEFAULT_GET_POLL_INTERVAL    100   /* default get poll interval set at 100 ms */
#define SABERTOOTH_DEFAULT_GET_TIMEOUT          3000  /* default get timeout set at 3 seconds */
#define SABERTOOTH_GET_TIMED_OUT               -32768
#define SABERTOOTH_GET_ERROR                   -32767
#define SABERTOOTH_GET_BUSY                    -32766
#define SABERTOOTH_INFINITE_TIMEOUT            -1
#define SABERTOOTH_MAX_VALUE                    16383

enum USBSabertoothCommand
{
  SABERTOOTH_CMD_SET = 40,
  SABERTOOTH_CMD_GET = 41,
};

enum USBSabertoothReplyCode
{
  SABERTOOTH_RC_GET = 73
};

enum USBSabertoothGetType
{
  SABERTOOTH_GET_VALUE       = 0x00,
  SABERTOOTH_GET_BATTERY     = 0x10,
  SABERTOOTH_GET_CURRENT     = 0x20,
  SABERTOOTH_GET_TEMPERATURE = 0x40
};

enum USBSabertoothSetType
{
  SABERTOOTH_SET_VALUE     = 0x00,
  SABERTOOTH_SET_KEEPALIVE = 0x10,
  SABERTOOTH_SET_SHUTDOWN  = 0x20,
  SABERTOOTH_SET_TIMEOUT   = 0x40
};

class USBSabertoothCommandWriter
{
public:
  static size_t writeToBuffer(byte* buffer, byte address, USBSabertoothCommand command, boolean useCRC, const byte* data, size_t lengthOfData);
};

class USBSabertoothChecksum
{
public:
  static byte value(const byte* data, size_t lengthOfData);
};

class USBSabertoothCRC7
{
public:
  void begin();
  void write(byte data);
  void write(const byte* data, size_t lengthOfData);
  void end  ();
  
public:
  inline byte value() const   { return _crc; }
         void value(byte crc) { _crc = crc;  }
  
  static byte value(const byte* data, size_t lengthOfData);
  
private:
  byte _crc;
};

class USBSabertoothCRC14
{
public:
  void begin();
  void write(byte data);
  void write(const byte* data, size_t lengthOfData);
  void end  ();
  
public:
  inline uint16_t value() const       { return _crc; }
         void     value(uint16_t crc) { _crc = crc;  }
  
  static uint16_t value(const byte* data, size_t lengthOfData);
  
private:
  uint16_t _crc;
};

class USBSabertoothTimeout
{
public:
  USBSabertoothTimeout(int32_t timeoutMS);
  
public:
  boolean canExpire() const;
  boolean expired() const;
  void expire();
  void reset();

public:
  inline void setTimeoutMS( int32_t interval ) { _timeoutMS = interval; }
  inline int32_t timeoutMS() const { return _timeoutMS; }  
  
private:
  uint32_t _start;
  int32_t  _timeoutMS;
};

class USBSabertoothReplyReceiver
{
public:
  USBSabertoothReplyReceiver();
  
public:
  inline       byte                   address () const { return _data[0];                         }
  inline       USBSabertoothReplyCode command () const { return (USBSabertoothReplyCode)_data[1]; }
  inline const byte*                  data    () const { return _data;                            }
  inline       boolean                usingCRC() const { return _usingCRC;                        }
  
public:
  inline boolean ready() const { return _ready; }
         void    read (byte data);
         void    reset();
  
private:
  byte    _data[SABERTOOTH_COMMAND_MAX_BUFFER_LENGTH];
  size_t  _length;
  boolean _ready, _usingCRC;
};

struct USBSabertoothRequest
{ 
  byte           commandData[SABERTOOTH_GETCOMMAND_DATA_LENGTH];
  int            context;
  byte           address;
  boolean        crc;
  
  USBSabertoothRequest() : _pending(false), _timeout(SABERTOOTH_DEFAULT_GET_TIMEOUT) {  _timeout.expire(); }

  inline uint32_t  timeoutMS() const { return _timeout.timeoutMS(); }
  inline void      setTimeoutMS( uint32_t interval ) { _timeout.setTimeoutMS( interval );  }
  inline boolean   expired() const { return _timeout.expired(); }
  inline void      expire() { _timeout.expire(); _pending = false; }
  inline void      reset() { _timeout.reset(); _pending = true; }
  inline boolean   pending() const { return _pending; }

private:
  USBSabertoothTimeout  _timeout; 
  boolean               _pending; 
};

/*!
\class USBSabertoothSerial
\brief Create a USBSabertoothSerial for the serial port you are using, and then
       attach a USBSabertooth for each motor driver you want to communicate with.
*/
class USBSabertoothSerial
{
  friend class USBSabertooth;
  
public:
  /*!
  Constructs a USBSabertoothSerial object.
  \param port The serial port the motor driver is on.
              By default, this is the Arduino TX pin.
  */
  USBSabertoothSerial(Stream& port = SabertoothTXPinSerial);
  
public:
  /*!
  Gets the serial port being used.
  \return The serial port.
  */
  inline Stream& port() const { return _port; }

  /*!
  Checks whether a reply from any of the USBSabertooth async_get function calls is available. 
  Always returns immediatelly, must be called repeadly until it returns true either with a response or an error. 
  \param type (returned by reference) The type of channel to get from. See get function. Always updated
  \param number (returned by reference) The number of the channel, 1 or 2. Always updated
  \param getType (returned by reference) The Get command type. Always updated
  \param result (returned by reference) The resulting value, SABERTOOTH_GET_ERROR, or SABERTOOTH_GET_TIMED_OUT. 
        Only updated in case of success, i.e the funcion returning true, undefined otherwise
  \return true if new data is available or the get command timed out, false otherwhise 
  */
  boolean reply_available( byte *type, byte *number, USBSabertoothGetType *getType, int *result, int *context );
  boolean reply_available( byte *type, byte *number, int *result, int *context );
  boolean reply_available( byte *number, int *result, int *context );
  boolean reply_available( int *result, int *context );
  
  /*!
  Gets the poll interval.
  \return The poll interval, in milliseconds.
  */
  inline int32_t getPollInterval() const { return _poll.timeoutMS(); }
  
  /*!
  Sets the poll interval.
  \param timeoutMS The poll interval, in milliseconds.
  */
  inline void setPollInterval(int32_t timeoutMS) { _poll.setTimeoutMS(timeoutMS); }

  /*!
  Gets the get timeout.
  \return The get timeout, in milliseconds.
  */
  inline int32_t getGetTimeout() const { return _request.timeoutMS(); }
  
  /*!
  Sets the get timeout.
  \param timeoutMS The get timeout, in milliseconds.
  */
  inline void setGetTimeout(int32_t timeoutMS) { _request.setTimeoutMS(timeoutMS); }

private:
  void    write    (byte address, USBSabertoothCommand command, boolean useCRC, const byte* data, size_t lengthOfData);
  int     get      (byte address, boolean useCrc, byte type, byte number, USBSabertoothGetType getType, boolean unescaled);
  boolean async_get(byte address, boolean useCrc, byte type, byte number, USBSabertoothGetType getType, int context, boolean unescaled);
  boolean tryReceivePacket();
  void    clearSerial();

private:
  USBSabertoothSerial(USBSabertoothSerial& serial); // no copy
  void operator =    (USBSabertoothSerial& serial);

private:
  USBSabertoothReplyReceiver _receiver;
  USBSabertoothRequest       _request;
  USBSabertoothTimeout       _poll;
  Stream&                    _port;
};

/*!
\class USBSabertooth
\brief Controls a USB Sabertooth motor driver running in Packet Serial mode.
*/
class USBSabertooth
{
public:
  /*!
  Initializes a new instance of the USBSabertooth class.
  The driver address is set to the value given, and the specified serial port is used.
  \param serial The USBSabertoothSerial whose serial port the motor driver is on.
  \param address The driver address.
  */
  USBSabertooth(USBSabertoothSerial& serial, byte address);
  
public:
  /*!
  Gets the driver address.
  \return The driver address.
  */
  inline byte address() const { return _address; }

  /*!
  Sends a packet serial command to the motor driver.
  \param command The number of the command.
  \param value   The command's value.
  */
  void command(USBSabertoothCommand cmd, byte value);
  
  /*!
  Sends a multibyte packet serial command to the motor driver.
  \param command The number of the command.
  \param value   The command's value.
  \param bytes   The number of bytes in the value.
  */
  void command(USBSabertoothCommand cmd, const byte* value, size_t bytes);
  
public:
  /*!
  Controls motor output 1.
  In User Mode, this sets M1.
  \param value The value, between -2047 and 2047.
  */
  void motor(int value);
  
  /*!
  Controls the specified motor output.
  In User Mode, this sets M1 or M2.
  \param motorOutputNumber The motor output number, 1 or 2.
                           You can also use a character, such as '3', to select the
                           motor output by its Plain Text Serial address.
  \param value             The value, between -2047 and 2047.
  */
  void motor(byte motorOutputNumber, int value);
  
  /*!
  Controls power output 1, if power output 1 is configured as a controllable output.
  In User Mode, this sets P1.
  \param value The value, between -2047 and 2047.
  */
  void power(int value);
  
  /*!
  Controls the specified power output, if the power output is configured as a controllable output.
  In User Mode, this sets P1 or P2.
  \param powerOutputNumber The power output number, 1 or 2.
                           You can also use a character, such as '3', to select the
                           power output by its Plain Text Serial address.
  \param value             The value, between -2047 and 2047.
  */
  void power(byte powerOutputNumber, int value);
  
  /*!
  Controls the mixed-mode drive channel.
  In User Mode, this sets MD.
  \param value The value, between -2047 and 2047.
  */
  void drive(int value);
  
  /*!
  Controls the mixed-mode turn channel.
  In User Mode, this sets MT.
  \param value The value, between -2047 and 2047.
  */
  void turn(int value);
  
  /*!
  Causes motor output 1 to freewheel.
  In User Mode, this sets Q1.
  \param value  true or a positive value lets the motor outputs freewheel.
                false or a negative or zero value stops the freewheeling.
  */
  void freewheel(int value = 2048);
  
  /*!
  Causes the specified motor output to freewheel.
  In User Mode, this sets Q1 or Q2.
  \param motorOutputNumber The motor output number, 1 or 2.
                           You can also use a character, such as '3', to select the
                           motor output by its Plain Text Serial address.
  \param value             true or a positive value lets the motor output freewheel.
                           false or a negative or zero value stops the freewheeling.
  */
  void freewheel(byte motorOutputNumber, int value = 2048);
  
  /*!
  Shuts down an output.
  \param type   The type of output to shut down. This can be
                'M' (motor output) or
                'P' (power output).
  \param number The number of the output, 1 or 2.
                You can also use a character, such as '3', to select by
                Plain Text Serial address.
  \param value  true sets the shutdown.
                false clears the shutdown.
  */
  void shutDown(byte type, byte number, boolean value = true);
  
public:
  /*!
  Sets a value on the motor driver.
  \param type   The type of channel to set. This can be
                'M' (motor output),
                'P' (power output),
                'Q' (freewheel), or
                'R' (ramping).
  \param number The number of the channel, 1 or 2.
                You can also use a character, such as '3', to select by
                Plain Text Serial address.
  \param value  The value, between -16383 and 16383
                (though in many cases, only -2047 to 2047 are meaningful).
  */
  void set(byte type, byte number, int value);
  
  /*!
  Sets the ramping for all motor outputs.
  In User Mode, this sets R1 and R2.
  \param value The ramping value, between -16383 (fast) and 2047 (slow).
  */
  void setRamping(int value);
  
  /*!
  Sets the ramping for the specified motor output.
  In User Mode, this sets R1 or R2.
  \param motorOutputNumber The motor output number, 1 or 2.
                           You can also use a character, such as '3', to select the
                           motor output by its Plain Text Serial address.
  \param value             The ramping value, between -16383 (fast) and 2047 (slow).
  */
  void setRamping(byte motorOutputNumber, int value);
    
  /*!
  Sets the serial timeout.
  \param milliseconds The maximum time in milliseconds between packets.
                      If this time is exceeded, the driver will stop the
                      motor and power outputs.
                      A value of zero uses the DEScribe setting.
                      SABERTOOTH_INFINITE_TIMEOUT disables the timeout.
  */
  void setTimeout(int milliseconds);
  
  /*!
  Resets the serial timeout.
  This is done automatically any time a motor output is set.
  You can, however, call this if you don't want to set any motor outputs.
  */
  void keepAlive();
  
public: 
  /*!
  Gets a value from the motor driver.
  \param type  The type of channel to get from. This can be
                'S' (signal),
                'A' (aux),
                'M' (motor output), or
                'P' (power output).
  \param number The number of the channel, 1 or 2.
                You can also use a character, such as '3', to select by
                Plain Text Serial address.
  \return The value, or SABERTOOTH_GET_TIMED_OUT.
  */
  inline int get(byte type, byte number)
  {
    return get(type, number, SABERTOOTH_GET_VALUE, false);
  }

  /*!
  Gets the battery voltage.
  \param motorOutputNumber The number of the motor output, 1 or 2.
                           You can also use a character, such as '3', to select by
                           Plain Text Serial address.
  \param unscaled          If true, gets in unscaled units. If false, gets in scaled units.
  \return The value, or SABERTOOTH_GET_TIMED_OUT.
  */
  inline int getBattery(byte motorOutputNumber, boolean unscaled = false)
  {
    return get('M', motorOutputNumber, SABERTOOTH_GET_BATTERY, unscaled);
  }
  
  /*!
  Gets the motor output current.
  \param motorOutputNumber The number of the motor output, 1 or 2.
                           You can also use a character, such as '3', to select by
                           Plain Text Serial address.
  \param unscaled          If true, gets in unscaled units. If false, gets in scaled units.
  \return The value, or SABERTOOTH_GET_TIMED_OUT.
  */
  inline int getCurrent(byte motorOutputNumber, boolean unscaled = false)
  {
    return get('M', motorOutputNumber, SABERTOOTH_GET_CURRENT, unscaled);
  }
  
  /*!
  Gets the motor output temperature.
  \param motorOutputNumber The number of the motor output, 1 or 2.
                           You can also use a character, such as '3', to select by
                           Plain Text Serial address.
  \param unscaled          If true, gets in unscaled units. If false, gets in scaled units.
  \return The value, or SABERTOOTH_GET_TIMED_OUT.
  */
  inline int getTemperature(byte motorOutputNumber, boolean unscaled = false)
  {
    return get('M', motorOutputNumber, SABERTOOTH_GET_TEMPERATURE, unscaled);
  }

  /*!
  Asynchronous, non blocking, get functions. After calling one of these, use one of the 'reply_available'
  methods of the USBSabertoothSerial to get the response.
  \param type    See get method.
  \param number  See get method.
  \param context Any arbitrary number. It will be returned by 'reply_available' upon reception of the response
  \return true if the get command was sent, false if still waiting response from the previous call. 
        These functions will always return immediatelly
  */

  inline boolean async_get(byte type, byte number, int context=0) {
    return async_get(type, number, SABERTOOTH_GET_VALUE, context, false);
  }

  inline boolean async_getBattery(byte motorOutputNumber, int context=0, boolean unscaled = false) {
    return async_get('M', motorOutputNumber, SABERTOOTH_GET_BATTERY, context, unscaled);
  }
  
  inline boolean async_getCurrent(byte motorOutputNumber, int context=0, boolean unscaled = false) {
    return async_get('M', motorOutputNumber, SABERTOOTH_GET_CURRENT, context, unscaled);
  }
  
  inline boolean async_getTemperature(byte motorOutputNumber, int context=0, boolean unscaled=false ) {
    return async_get('M', motorOutputNumber, SABERTOOTH_GET_TEMPERATURE, context, unscaled);
  }
  
public:
  /*! Gets the get retry interval.
  \return The get retry interval, in milliseconds.
  */
  //inline int32_t getGetRetryInterval() const { return _getRetryInterval; }
  
  /*!
  Sets the get retry interval.
  \param intervalMS The command retry interval, in milliseconds.
  */
  //inline void setGetRetryInterval(int32_t intervalMS) { _getRetryInterval = intervalMS; }

  /*!
  Gets the get timeout. DEPRECATED, use the USBSabertoothSerial functions instead
  \return The get timeout, in milliseconds.
  */
  inline int32_t getGetTimeout() const { return _serial._request.timeoutMS(); }
  
  /*!
  Sets the get timeout. DEPRECATED, use the USBSabertoothSerial functions instead
  \param timeoutMS The get timeout, in milliseconds.
  */
  inline void setGetTimeout(int32_t timeoutMS) { _serial._request.setTimeoutMS(timeoutMS); }

  /*!
  Gets whether CRC-protected commands are used. They are, by default.
  \return True if CRC-protected commands are used.
  */  
  inline boolean usingCRC() const { return  _crc; }
  
  /*!
  Causes future commands to be sent CRC-protected (larger packets, excellent error detection).
  */
  inline void useChecksum() { _crc = false; }
  
  /*!
  Causes future commands to be sent checksum-protected (smaller packets, reasonable error detection).
  */
  inline void useCRC() { _crc = true ; }
  
private:
  int get(byte type, byte number,
          USBSabertoothGetType getType, boolean unscaled);
  
  boolean async_get(byte type, byte number,
          USBSabertoothGetType getType, int context, boolean unscaled);
          
  void set(byte type, byte number, int value,
            USBSabertoothSetType setType);
  
private:
  const byte           _address;
  boolean              _crc;
  USBSabertoothSerial& _serial;
};

#endif
