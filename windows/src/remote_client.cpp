#include "remote_client.h"
#include "win32/immdev.h"
#include "auto_lock.h"

void _updateStatusBar(DroidPad::CIMEStatusBar *pBar, DroidPad::eBarState state)
{
  if (!pBar)
    return;
  switch (state)
  {
  case DroidPad::STATE_CONNECTED_BY_USB:
    pBar->IMEOnDeviceConnected(DroidPad::CNN_TYPE_USB);
    break;
  case DroidPad::STATE_CONNECTED_BY_WIFI:
    pBar->IMEOnDeviceConnected(DroidPad::CNN_TYPE_WIFI);
    break;
  case DroidPad::STATE_CONNECTING:
    pBar->IMEOnDeviceConnecting();
    break;
  case DroidPad::STATE_CONNECTION_FAIL:
    pBar->IMEOnDeviceConnectionFail();
    break;
  case DroidPad::STATE_NOT_CONNECTED:
    pBar->IMEOnDeviceNone();
    break;
  }
}

/////////////////////////////////////////////////////////

namespace DroidPad
{

CRemoteClient::CRemoteClient(const SESSIONID szId) : CIPCThread(szId.c_str(), true),
                                                     m_szId(szId),
                                                     m_pStatusBar(NULL),
                                                     m_eBarState(STATE_NOT_CONNECTED)
{
  InitializeCriticalSection(&cs);
}

CRemoteClient::~CRemoteClient()
{
  DeleteCriticalSection(&cs);
}

bool CRemoteClient::OnCreateThread()
{
  CIMEStatusBar *bar = new CIMEStatusBar();
  bar->setCallback(this);
  if (!bar->initialize() || !bar->resumeThread())
  {
    delete bar;
    return false;
  }
  return true;
}

void CRemoteClient::OnExitThread()
{
  if (m_pStatusBar && m_pStatusBar->isAlive())
    m_pStatusBar->terminate();
}

void CRemoteClient::OnDestroyThread()
{
  if (m_pStatusBar)
    delete m_pStatusBar;
  m_pStatusBar = NULL;
}

void CRemoteClient::OnIMEStatusBarError(std::wstring &err)
{
  this->IMEOnError(err);
}

void CRemoteClient::OnIMEStatusBarReady(CIMEStatusBar *pBar)
{
  CAutoLock _l(&cs);
  m_pStatusBar = pBar;
  _updateStatusBar(m_pStatusBar, m_eBarState);
}

void CRemoteClient::IMEOnClientActive(bool active)
{
  {
    CAutoLock _l(&cs);
    if (m_pStatusBar)
      m_pStatusBar->showBar(active);
  }
  PACKET p;
  p.setCMD(MIME_CLIENT_ACTIVE);
  p.write(active);
  send(p, NULL);
}

void CRemoteClient::IMEOnClientSelect(bool select)
{
  {
    CAutoLock _l(&cs);
    if (m_pStatusBar)
      m_pStatusBar->showBar(select);
  }
  PACKET p;
  p.setCMD(MIME_CLIENT_SELECT);
  p.write(select);
  send(p, NULL);
}

void CRemoteClient::IMEOnError(std::wstring &error)
{
  PACKET p;
  p.setCMD(MIME_STATUS_BAR_ERROR);
  p.write(error);
  send(p, NULL);
}

std::wstring CRemoteClient::getWordSet()
{
  std::wstring ret;
  CAutoLock _l(&cs);
  if (!m_composedWords.empty())
  {
    ret = m_composedWords[0];
    m_composedWords.erase(m_composedWords.begin());
  }
  return ret;
}

void CRemoteClient::showMessage(LPCWSTR msg)
{
  if (m_pStatusBar)
    m_pStatusBar->showMessage(msg);
}

void CRemoteClient::IMEOnNewWords(std::wstring words)
{
  {
    CAutoLock _l(&cs);
    m_composedWords.push_back(words);
  }
  INPUT inputs[2];
  memset(inputs, 0, sizeof(inputs));
  inputs[0].type = INPUT_KEYBOARD;
  inputs[0].ki.wVk = VK_PROCESSKEY;
  inputs[0].ki.wScan = 0;
  inputs[0].ki.dwFlags = 0;

  inputs[1].type = INPUT_KEYBOARD;
  inputs[1].ki.wVk = VK_PROCESSKEY;
  inputs[1].ki.wScan = 0;
  inputs[1].ki.dwFlags = KEYEVENTF_KEYUP;
  SendInput(2, inputs, sizeof(INPUT));
}

void CRemoteClient::IMEOnDeviceConnected(eConnType type)
{
  CAutoLock _l(&cs);
  m_eBarState = (type == CNN_TYPE_WIFI ? STATE_CONNECTED_BY_WIFI : STATE_CONNECTED_BY_USB);
  _updateStatusBar(m_pStatusBar, m_eBarState);
}

void CRemoteClient::IMEOnDeviceConnecting()
{
  CAutoLock _l(&cs);
  m_eBarState = STATE_CONNECTING;
  _updateStatusBar(m_pStatusBar, m_eBarState);
}

void CRemoteClient::IMEOnDeviceConnectionFail()
{
  CAutoLock _l(&cs);
  m_eBarState = STATE_CONNECTION_FAIL;
  _updateStatusBar(m_pStatusBar, m_eBarState);
}

void CRemoteClient::IMEOnDeviceNone()
{
  CAutoLock _l(&cs);
  m_eBarState = STATE_NOT_CONNECTED;
  _updateStatusBar(m_pStatusBar, m_eBarState);
}

void CRemoteClient::OnDataReceived(PACKET &data, PACKET *reply)
{
  switch (data.getCMD())
  {
  case MIME_NEW_CHAR:
  {
    std::wstring wstr;
    data.read(wstr);
    IMEOnNewWords(wstr);
    break;
  }
  case MIME_DEVICE_CONNECTED:
  {
    eConnType type;
    data.read(type);
    IMEOnDeviceConnected(type);
    break;
  }
  case MIME_DEVICE_CONNECTING:
    IMEOnDeviceConnecting();
    break;
  case MIME_DEVICE_NONE:
    IMEOnDeviceNone();
    break;
  case MIME_DEVICE_OFFLINE:
    IMEOnDeviceConnectionFail();
    break;
  default:
    CIPCThread::OnDataReceived(data, reply);
  }
}

} // namespace DroidPad