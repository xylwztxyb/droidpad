#ifndef __IME_STATUS_BAR_H__
#define __IME_STATUS_BAR_H__

#include "common.h"
#include "thread_base.h"
#include "image_cache.h"
#include "ime_interface.h"

namespace DroidPad
{

class CIMEStatusBar : public CThreadBase, public CIMEDeviceStateInterface
{
public:
  class StatusBarCallback
  {
  public:
    virtual void OnIMEStatusBarReady(CIMEStatusBar *pBar) = 0;
    virtual void OnIMEStatusBarError(std::wstring &err) = 0;
  };
public:
  CIMEStatusBar();
  ~CIMEStatusBar();

  virtual void IMEOnDeviceConnected(eConnType type) override;
  virtual void IMEOnDeviceConnectionFail() override;
  virtual void IMEOnDeviceConnecting() override;
  virtual void IMEOnDeviceNone() override;

  void showBar(bool bShow);
  void showMessage(LPCTSTR msg);
  void setCallback(StatusBarCallback *callback)
  {m_pCallback = callback;}

private:
  virtual void OnExitThread() override;
  virtual bool threadLoop() override;
  
  HWND m_hWnd;
  HINSTANCE m_hInst;
  HANDLE m_hWinCreated;
  eBarState m_eCurState;
  StatusBarCallback *m_pCallback;
};

}
#endif