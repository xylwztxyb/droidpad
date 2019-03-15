#ifndef __thread_base_h__
#define __thread_base_h__

#include "common.h"

namespace DroidPad
{

class CThreadBase
{
public:
  CThreadBase();
  virtual ~CThreadBase();
  bool initialize();
  void terminate();
  bool resumeThread();
  void suspendThread();
  bool isAlive() { return m_bAlive; }

protected:
  virtual bool OnCreateThread();
  virtual void OnStartThread();
  virtual void OnExitThread();
  virtual void OnDestroyThread();
  virtual bool threadLoop() = 0;
  void suspendInner();

private:
  static unsigned int __stdcall _threadEntry(void *args);
  HANDLE m_hThreadHandle;
  unsigned int m_iThreadId;
  bool m_bInited;
  bool m_bPendingExit;
  bool m_bPendingSuspend;
  bool m_bAlive;
  bool m_bSuspended;
  HANDLE m_hWakeUpEvent;
  CRITICAL_SECTION m_cs;
};

}

#endif // thread_base_h__