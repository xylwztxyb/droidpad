#ifndef __IME_INTERFACE_H__
#define __IME_INTERFACE_H__

#include "common.h"

namespace DroidPad
{


class CIMEInterface
{
public:
  virtual void IMEOnClientActive(bool bActive) = 0;
  virtual void IMEOnClientSelect(bool bSelect) = 0;
  virtual void IMEOnNewWords(std::wstring words) = 0;
  virtual void IMEOnError(std::wstring &error) = 0;
};

class CIMEDeviceStateInterface
{
  virtual void IMEOnDeviceConnected(eConnType type) = 0;
  virtual void IMEOnDeviceConnectionFail() = 0;
  virtual void IMEOnDeviceConnecting() = 0;
  virtual void IMEOnDeviceNone() = 0;
};

}

#endif
