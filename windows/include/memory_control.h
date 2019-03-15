#ifndef __MEMORY_CONTROL_H__
#define __MEMORY_CONTROL_H__

#include "common.h"
namespace DroidPad
{

struct SMemCtrlBlk
{
public:
  SMemCtrlBlk(LPCWSTR szId);
  ~SMemCtrlBlk();
  eSTATUS init(bool bCreate);
  void cleanEventState();

  DWORD wait_read_done(u_long ulTimeout);
  void signal_read_done();
  DWORD wait_write_done(u_long ulTimeout);
  void signal_write_done();

  void _lock();
  void _release();


private:
  LPWSTR m_szIdentify;
  HANDLE m_hMutex;
  HANDLE m_hEvent_Read_Done;
  HANDLE m_hEvent_Write_Done;
};

}

#endif