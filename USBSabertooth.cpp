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

#include "USBSabertooth_NB.h"

USBSabertooth::USBSabertooth(USBSabertoothSerial& serial, byte address)
  : _address(address), _serial(serial)
{
  init();
}

void USBSabertooth::init()
{
  useCRC();
  //setGetRetryInterval(SABERTOOTH_DEFAULT_GET_RETRY_INTERVAL);
  setPollInterval(SABERTOOTH_DEFAULT_GET_POLL_INTERVAL);
  setGetTimeout(SABERTOOTH_DEFAULT_GET_TIMEOUT);
}

void USBSabertooth::command(byte command,
                            byte value)
{
  this->command(command, &value, 1);
}

void USBSabertooth::command(byte command,
                            const byte* value, size_t bytes)
{
  USBSabertoothCommandWriter::writeToStream(_serial.port(), address(),
                                            (USBSabertoothCommand)command,
                                            usingCRC(), value, bytes);
}

void USBSabertooth::motor(int value)
{
  motor(1, value);
}

void USBSabertooth::motor(byte number, int value)
{
  set('M', number, value);
}

void USBSabertooth::power(int value)
{
  power(1, value);
}

void USBSabertooth::power(byte number, int value)
{
  set('P', number, value);
}

void USBSabertooth::drive(int value)
{
  motor('D', value);
}

void USBSabertooth::turn(int value)
{
  motor('T', value);
}

void USBSabertooth::freewheel(int value)
{
  freewheel(1, value);
}

void USBSabertooth::freewheel(byte number, int value)
{
  set('Q', number, value);
}

void USBSabertooth::shutDown(byte type, byte number, boolean value)
{
  set(type, number, value ? 2048 : 0, SABERTOOTH_SET_SHUTDOWN);
}

void USBSabertooth::setRamping(int value)
{
  setRamping('*', value);
}

void USBSabertooth::setRamping(byte number, int value)
{
  set('R', number, value);
}

void USBSabertooth::setTimeout(int milliseconds)
{
  set('M', '*', milliseconds, SABERTOOTH_SET_TIMEOUT);
}

void USBSabertooth::keepAlive()
{
  set('M', '*', 0, SABERTOOTH_SET_KEEPALIVE);
}


void USBSabertooth::set(byte type, byte number, int value)
{
  set(type, number, value, SABERTOOTH_SET_VALUE);
}

void USBSabertooth::set(byte type, byte number, int value,
                         USBSabertoothSetType setType)
{
  byte flags = (byte)setType;
  if (value < -SABERTOOTH_MAX_VALUE) { value = -SABERTOOTH_MAX_VALUE; }
  if (value >  SABERTOOTH_MAX_VALUE) { value =  SABERTOOTH_MAX_VALUE; }
  if (value <                     0) { value = -value;   flags |=  1; }
  
  byte data[5];
  data[0] = flags;
  data[1] = (byte)((uint16_t)value >> 0) & 0x7f;
  data[2] = (byte)((uint16_t)value >> 7) & 0x7f;
  data[3] = type;
  data[4] = number;
  
  command(SABERTOOTH_CMD_SET, data, sizeof(data));
}

/*
int USBSabertooth::get(byte type, byte number,
                       USBSabertoothGetType getType, boolean unscaled)
{
  USBSabertoothTimeout timeout(getGetTimeout());
  USBSabertoothTimeout retry(getGetRetryInterval()); retry.expire();
  
  byte flags = (byte)getType;
  if (unscaled) { flags |= 2; }
  
  USBSabertoothReplyReceiver receiver;
  USBSabertoothReplyCode replyCode = SABERTOOTH_RC_GET;
  
  while (1)
  {
    if (timeout.expired())
    {
      return SABERTOOTH_GET_TIMED_OUT;
    }
    
    if (retry.expired())
    {
      retry.reset();
      
      byte data[3];
      data[0] = flags;
      data[1] = type;
      data[2] = number;
      
      command(SABERTOOTH_CMD_GET,
              data, sizeof(data));
    }
    
    if (!_serial.tryReceivePacket ()              ) { continue; }
    if (_serial._receiver.address () != address() ) { continue; }
    if (_serial._receiver.command () != replyCode ) { continue; }
    if (_serial._receiver.usingCRC() != usingCRC()) { continue; }
    
    const byte* data = _serial._receiver.data();
    if (flags  == (data[2] & ~1) &&
        type   ==  data[6]       &&
        number ==  data[7])
    {
      int16_t value = (uint16_t)data[4] << 0 |
                      (uint16_t)data[5] << 7 ;
      return (data[2] & 1) ? -value : value;
    }
  }
}
*/

int USBSabertooth::get(byte type, byte number,
                       USBSabertoothGetType getType, boolean unscaled)
{
  _reader.pollExpire();  // make sure the command is executed as soon as possible
  
  if ( !async_get( type, number, getType, 0, unscaled ) )
    return SABERTOOTH_GET_BUSY;   // will return immediatelly if get command is in progress, this may happen when improperly mixing asynchronous and and synchronous calls 

  int result, context;
  while ( ! reply_available( &result, &context ) ) { }    // wait until the reply comes

  return result;   
}

boolean USBSabertooth::async_get(byte type, byte number,
                       USBSabertoothGetType getType, int context, boolean unscaled)
{
  // we do no accept any new get command until the previous one is completed
  if ( _reader.waiting() )  
    return false;
  
  byte flags = (byte)getType;
  if (unscaled) { flags |= 2; }

  // store command data including context
  byte *commandData = _reader.commandData();
  commandData[0] = flags;
  commandData[1] = type;
  commandData[2] = number;
  _reader.setContext( context );

  // if user has polling disabled send the command right now 
  if ( !_reader.isPolling() )  
  {
    _reader.reset();
    command(SABERTOOTH_CMD_GET,
           commandData, SABERTOOTH_GETCOMMAND_DATA_LENGTH);
  }

  // we accepted the command, so return true
  return true;
}

boolean USBSabertooth::reply_available( byte *type, byte *number, USBSabertoothGetType *getType, int *result, int *context)
{
  const byte* commandData = _reader.commandData();

  // retrieve get command data including the context for the user
  *getType = (USBSabertoothGetType)(commandData[0] & ~3) ;
  *type = commandData[1];
  *number = commandData[2];
  *context = _reader.context();

  // it might be time to send the command, so do it now and return
  if ( !_reader.waiting() && _reader.pollExpired() )  // send now the next command if it's time to do so
  {
      _reader.pollReset();
      _reader.reset();
      command(SABERTOOTH_CMD_GET,
             commandData, SABERTOOTH_GETCOMMAND_DATA_LENGTH);
      return false;
  }

  // otherwise the actual reply processing starts here 
  USBSabertoothReplyReceiver &receiver = _serial._receiver;
  
  // handle timeout
  if (_reader.expired()) 
  {
    *result = SABERTOOTH_GET_TIMED_OUT;
  }
  else
  {
    // check for reply packets ready to use, return false if not yet available
    if (!_serial.tryReceivePacket () ) return false;

    // at this point we have a reply packet waiting for processing
    const USBSabertoothReplyCode replyCode = SABERTOOTH_RC_GET;

    // check whether the packet was intended to us, return false otherwhise as another USBSabertooth object might handle it
    if ( receiver.address () != _address ||
         receiver.command () != replyCode ||
         receiver.usingCRC() != _crc ) return false;
  
    // parse result or return error if not possible
    const byte* data = receiver.data();
    if (  commandData[0]  == (data[2] & ~1) &&
          commandData[1]  ==  data[6] &&
          commandData[2]  ==  data[7] )
    {
       int16_t value = (uint16_t)data[4] << 0 | (uint16_t)data[5] << 7;
      *result = ((data[2] & 1) ? -value : value );
    } else *result = SABERTOOTH_GET_ERROR; 
  }

  // we got a packet processed, stop the timeout timer, reset the receiver and return true
  _reader.expire();
  receiver.reset();
  return true;
}

boolean USBSabertooth::reply_available( byte *type, byte *number, int *result, int *context )
{
  USBSabertoothGetType getType;
  return reply_available( type, number, &getType, result, context );
}

boolean USBSabertooth::reply_available( byte *number, int *result, int *context )
{
  byte type;
  return reply_available( &type, number, result, context ) ;
}

boolean USBSabertooth::reply_available( int *result, int *context )
{
  byte type, number; 
  USBSabertoothGetType getType;
  return reply_available( &type, &number, &getType, result, context );
}
