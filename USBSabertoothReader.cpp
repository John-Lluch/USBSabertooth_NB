
#include "USBSabertooth_NB.h"

USBSabertoothReader::USBSabertoothReader()
    : _timeout(SABERTOOTH_DEFAULT_GET_TIMEOUT), _poll(SABERTOOTH_DEFAULT_GET_POLL_INTERVAL)
{
   _waiting = false;
   _timeout.expire();
   _poll.expire();
}

void USBSabertoothReader::expire()
{
  _waiting = false;
  _timeout.expire();
}

void USBSabertoothReader::reset()
{
  _waiting = true;
  _timeout.reset();
}

void USBSabertoothReader::pollExpire()
{
  _poll.expire();
}

void USBSabertoothReader::pollReset()
{
  _poll.reset();
}
