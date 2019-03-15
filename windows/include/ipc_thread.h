#ifndef __IPC_THREAD_H__
#define __IPC_THREAD_H__

#include "common.h"
#include "channel.h"
#include "thread_base.h"

namespace DroidPad
{

class CIPCThread : protected CChannelBase::CallBack, public CThreadBase
{
public:
  CIPCThread(LPCWSTR szId, bool bClient);
  virtual ~CIPCThread();
  eSTATUS send(const PACKET data, PACKET *reply);
  //void getSessionId(LPWSTR id, size_t lenInWords);

protected:
  virtual void OnDataReceived(PACKET &data, PACKET *reply) override;
  virtual void OnIOTimeout();
  virtual bool threadLoop() override;

private:
  CChannelBase *m_pChannel;
  bool m_bClient;
 // SESSIONID m_szSessionId;
};

}

#endif