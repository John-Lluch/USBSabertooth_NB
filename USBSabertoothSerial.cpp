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

USBSabertoothSerial::USBSabertoothSerial(Stream& port)
  : _port(port), _poll(SABERTOOTH_DEFAULT_GET_POLL_INTERVAL)
{
  setGetTimeout(SABERTOOTH_DEFAULT_GET_TIMEOUT);
  _poll.expire();
}

void USBSabertoothSerial::clearSerial()
{
  while( !( _port.read() < 0) );
}

boolean USBSabertoothSerial::tryReceivePacket()
{  
  while ( !_receiver.ready() )    // do not attempt to read any further bytes unless the reveiver is reset
  {
    int value = _port.read();
    if (value < 0) { return false; }

    _receiver.read((byte)value);
  }
  return true;
}

void USBSabertoothSerial::write( byte address, USBSabertoothCommand command, boolean useCRC,
                                               const byte* data, size_t lengthOfData)
{
  byte buffer[SABERTOOTH_COMMAND_MAX_BUFFER_LENGTH];
  size_t lengthOfBuffer = USBSabertoothCommandWriter::writeToBuffer(buffer, address, command, useCRC, data, lengthOfData);
  _port.write(buffer, lengthOfBuffer);
}

void USBSabertoothSerial::set(byte address, boolean useCrc, byte type, byte number, 
                    int value, USBSabertoothSetType setType)
{
  byte flags = (byte)setType;
  if (value < -SABERTOOTH_MAX_VALUE) { value = -SABERTOOTH_MAX_VALUE; }
  if (value >  SABERTOOTH_MAX_VALUE) { value =  SABERTOOTH_MAX_VALUE; }
  if (value <                     0) { value = -value;   flags |=  1; }
  
  byte commandData[5];
  commandData[0] = flags;
  commandData[1] = (byte)((uint16_t)value >> 0) & 0x7f;
  commandData[2] = (byte)((uint16_t)value >> 7) & 0x7f;
  commandData[3] = type;
  commandData[4] = number;
  
  write( address, SABERTOOTH_CMD_SET, useCrc, commandData, sizeof(commandData)); 
}

int USBSabertoothSerial::get(byte address, boolean useCrc, byte type, byte number,
                       USBSabertoothGetType getType, boolean unscaled)
{  
  _poll.expire();  // make sure the command is sent as soon as possible 

  if ( !async_get( address, useCrc, type, number, getType, 0, unscaled ) )
    return SABERTOOTH_GET_BUSY;   // return immediatelly if get command is in progress, 
                                  // this may happen when improperly mixing asynchronous and synchronous calls 

  int result, context;
  while ( !reply_available( &result, &context ) ) { }    // wait until the reply comes

  return result;   
}

boolean USBSabertoothSerial::async_get(byte address, boolean useCrc, byte type, byte number,
                       USBSabertoothGetType getType, int context, boolean unscaled)
{
  // we will not accept any new get command until the previous one is completed
  if ( _request.pending() )  
    return false;
  
  byte flags = (byte)getType;
  if (unscaled) { flags |= 2; }

  // store request data including context info
  byte *commandData = _request.commandData;
  commandData[0] = flags;
  commandData[1] = type;
  commandData[2] = number;

  _request.context = context;
  _request.address = address;
  _request.crc = useCrc;

  // if user has polling disabled send the command right now 
  if ( !_poll.canExpire() )  
  {
    _request.reset();
    write( address, SABERTOOTH_CMD_GET, useCrc, commandData, SABERTOOTH_GETCOMMAND_DATA_LENGTH );
  }

  // we accepted the request, so return true
  return true;
}

boolean USBSabertoothSerial::reply_available( byte *type, byte *number, USBSabertoothGetType *getType, int *result, int *context)
{
  const byte* commandData = _request.commandData;

  // always retrieve get command data including the context so it is available to the user
  // regardless of the return condition
  *getType = (USBSabertoothGetType)(commandData[0] & ~3) ;
  *type = commandData[1];
  *number = commandData[2];
  *context = _request.context;

  // check whether we have a pending request
  if ( _request.pending() )
  {
    // handle timeout
    if ( _request.expired() ) 
    {
      *result = *context = SABERTOOTH_GET_TIMED_OUT;
    }
    else
    {
      // check for reply packets ready to use, return false if not yet available
      if ( !tryReceivePacket() ) return false;

      // at this point we have a reply packet waiting for processing
      // check consistency between sent and received packet
      const USBSabertoothReplyCode replyCode = SABERTOOTH_RC_GET;
      const byte* data = _receiver.data();

      bool ok = ( _receiver.address () == _request.address &&
                  _receiver.command () == replyCode &&
                  _receiver.usingCRC() == _request.crc );
              
      ok = ok && ( commandData[0]  == (data[2] & ~1) &&
                   commandData[1]  ==  data[6] &&
                   commandData[2]  ==  data[7] );

      // parse result or return error
      if ( ok ) 
      {
        int16_t value = (uint16_t)data[4] << 0 | (uint16_t)data[5] << 7;
        *result = ((data[2] & 1) ? -value : value );
      } 
      else 
      {
        clearSerial();
        *result = *context = SABERTOOTH_GET_ERROR;
      }
    }

    // we got a packet processed, stop the timeout timer, reset the receiver and return true
    _request.expire();
    _receiver.reset();
    return true;
  }
  
  // there's not a pending request, it might be now the time to send the current one according to the poll interval
  if ( _poll.expired() )
  {
    _poll.reset();
    _request.reset();
     write( _request.address, SABERTOOTH_CMD_GET, _request.crc, commandData, SABERTOOTH_GETCOMMAND_DATA_LENGTH );
  }

  // we did not process a request this time, so return false
  return false;
}

boolean USBSabertoothSerial::reply_available( byte *type, byte *number, int *result, int *context )
{
  USBSabertoothGetType getType;
  return reply_available( type, number, &getType, result, context );
}

boolean USBSabertoothSerial::reply_available( byte *number, int *result, int *context )
{
  byte type;
  return reply_available( &type, number, result, context ) ;
}

boolean USBSabertoothSerial::reply_available( int *result, int *context )
{
  byte type, number; 
  USBSabertoothGetType getType;
  return reply_available( &type, &number, &getType, result, context );
}
