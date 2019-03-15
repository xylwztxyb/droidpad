#ifndef __WRITING_PAD_SERVICE_H__
#define __WRITING_PAD_SERVICE_H__

#include "channel.h"

namespace DroidPad
{

class CWritingPadService
{
public:
  static CWritingPadService *getInstance();
  void Destroy();
  eSTATUS OpenSession(IN HANDLE handle, OUT SESSIONID* id);
  void CloseSession(IN SESSIONID id);

private:
  CWritingPadService();
  CWritingPadService(const CWritingPadService &) {}
  ~CWritingPadService();
  eSTATUS init();
  u_long getServerPid();
  CChannelBase *m_pChannel;
};

}

#endif