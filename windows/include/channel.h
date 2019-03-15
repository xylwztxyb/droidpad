#ifndef __CHANNEL_H__
#define __CHANNEL_H__

#include "common.h"
#include "packet.h"
#include "memory_control.h"

namespace DroidPad
{

#define CHANNEL_MEM_MAX sizeof(PACKET) * 2
#define CHANNEL_MEM_OFFSET CHANNEL_MEM_MAX / 2

#define CHANNEL_RECV_TIMEOUT 1000
//The send timeout should not be too small, else there maybe cause some problems
// out of mind.
#define CHANNEL_SEND_TIMEOUT 2000


class CChannelBase
{
public:
  class CallBack
  {
  public:
    virtual void OnDataReceived(PACKET &data, PACKET *reply) = 0;
  };

public:
  explicit CChannelBase(LPCWSTR szId, bool bClient = false);
  virtual ~CChannelBase();
  eSTATUS init();
  eSTATUS send(const PACKET &data, PACKET *reply);
  eSTATUS recv();
  void getIdentify(LPWSTR id, size_t len) const;
  void registerDataProc(CallBack *pFunc) { m_fnCallback = pFunc; }

private:
  HANDLE m_hMapFile;
  LPVOID m_lpMemBase;
  SMemCtrlBlk* m_CtrlBlk_Read;
  SMemCtrlBlk* m_CtrlBlk_Write;
  LPWSTR m_szIdentify;
  bool m_bClient;
  CallBack *m_fnCallback;
};

}

#endif