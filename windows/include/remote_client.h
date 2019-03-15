#ifndef __REMOTE_CLIENT_H__
#define __REMOTE_CLIENT_H__

#include "common.h"
#include "ipc_thread.h"
#include "ime_interface.h"
#include "ime_status_bar.h"
#include "win32/immdev.h"
#include <vector>

#pragma comment(lib, "imm32.lib")

namespace DroidPad
{

class CRemoteClient : public CIPCThread,
                      public CIMEInterface,
                      public CIMEDeviceStateInterface,
                      public CIMEStatusBar::StatusBarCallback
{
public:
  CRemoteClient(const SESSIONID szId);
  virtual ~CRemoteClient();

  virtual void IMEOnClientActive(bool active) override;
  virtual void IMEOnClientSelect(bool select) override;
  virtual void IMEOnError(std::wstring &error) override;

  virtual void IMEOnDeviceConnected(eConnType type) override;
  virtual void IMEOnDeviceConnectionFail() override;
  virtual void IMEOnDeviceConnecting() override;
  virtual void IMEOnDeviceNone() override;

  virtual void OnIMEStatusBarError(std::wstring &err) override;
  virtual void OnIMEStatusBarReady(CIMEStatusBar *pBar) override;

  std::wstring getWordSet();
  void showMessage(LPCWSTR msg);

private:
  void OnDataReceived(PACKET &data, PACKET *reply) override;
  virtual void IMEOnNewWords(std::wstring words) override;
  virtual bool OnCreateThread() override;
  virtual void OnExitThread() override;
  virtual void OnDestroyThread() override;

  SESSIONID m_szId;
  std::vector<std::wstring> m_composedWords;
  CIMEStatusBar *m_pStatusBar;
  CRITICAL_SECTION cs;
  eBarState m_eBarState;
};

} // namespace DroidPad
#endif