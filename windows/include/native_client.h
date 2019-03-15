#ifndef __NATIVE_CLIENT_H__
#define __NATIVE_CLIENT_H__

#include "ipc_thread.h"
#include "ime_interface.h"
#include "msg_queue.h"
#include "native_client_pool.h"

namespace DroidPad
{

class CNativeClient : public CIPCThread, public CIMEInterface, public CIMEDeviceStateInterface
{
  friend class CNativeClientPool;

public:
  static CNativeClient *create(const SESSIONID id, CMessageQueue *main = NULL, HANDLE handle = NULL);
  virtual ~CNativeClient();

  virtual void IMEOnNewWords(std::wstring words) override;

  virtual void IMEOnDeviceConnected(eConnType type) override;
  virtual void IMEOnDeviceConnectionFail() override;
  virtual void IMEOnDeviceConnecting() override;
  virtual void IMEOnDeviceNone() override;

  HANDLE getRemoteHandle() { return m_hRemoteProcessHandle; }
  void setRemoteHandle(HANDLE handle) { m_hRemoteProcessHandle = handle; }
  SESSIONID getSessionID() { return m_szId; }
  void setMessageQueue(CMessageQueue *queue) { m_pMainQueue = queue; }

  void terminate(); //Hide the super func

private:
  CNativeClient(const SESSIONID id, CNativeClientPool *pool = NULL,
               CMessageQueue *main = NULL, HANDLE handle = NULL);
  void OnDataReceived(PACKET &data, PACKET *reply) override;
  void OnIOTimeout() override;
  void OnExitThread() override;

  virtual void IMEOnClientActive(bool active) override;
  virtual void IMEOnClientSelect(bool select) override;
  virtual void IMEOnError(std::wstring &error) override;

  void destroy();
  CMessageQueue *m_pMainQueue;
  CNativeClientPool *m_pClientPool;
  SESSIONID m_szId;
  HANDLE m_hRemoteProcessHandle;
};

}

#endif