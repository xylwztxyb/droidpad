#ifndef __net_monitor_h__
#define __net_monitor_h__

#include "common.h"
#include "thread_base.h"
#include "msg_queue.h"

#pragma comment(lib, "ws2_32.lib")

namespace DroidPad
{

enum BCmd
{
  ARE_YOU_MIME_SERVER = 80,
  I_AM_MIME_SERVER
};

struct BroadcastPack
{
  BCmd cmd;
  int data;
};

class CNetMonitor : public CThreadBase
{
public:
  CNetMonitor(CMessageQueue *queue);
  ~CNetMonitor();

private:
  virtual bool threadLoop() override;
  virtual void OnDestroyThread() override;
  virtual bool OnCreateThread() override;

  CMessageQueue *m_pMainQueue;

  SOCKET adbSock;
  SOCKET BrdcstSock;
  SOCKET wifiSock;
};

}
#endif // net_monitor_h__