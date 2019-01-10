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
  : _address(address), _serial(serial), _crc(true)
{}

void USBSabertooth::command(USBSabertoothCommand cmd,
                            byte value)
{
  command(cmd, &value, 1);
}

void USBSabertooth::command(USBSabertoothCommand cmd,
                            const byte* commandData, size_t length)
{
  _serial.write( _address, cmd, _crc, commandData, length);                                          
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
  _serial.set( _address, _crc, type, number, value, setType); 
}

int USBSabertooth::get(byte type, byte number,
                       USBSabertoothGetType getType, boolean unscaled)
{
  return _serial.get( _address, _crc, type, number, getType, unscaled );
}

boolean USBSabertooth::async_get(byte type, byte number,
                       USBSabertoothGetType getType, int context, boolean unscaled)
{ 
  return _serial.async_get( _address, _crc, type, number, getType, context, unscaled );
}
