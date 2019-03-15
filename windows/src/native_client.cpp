#include "native_client.h"
#include "util.h"

namespace DroidPad
{

CNativeClient::CNativeClient(const SESSIONID szId, CNativeClientPool *pool,
                             CMessageQueue *main, HANDLE handle) : CIPCThread(szId.c_str(), false),
                                                                   m_pMainQueue(main),
                                                                   m_szId(szId),
                                                                   m_hRemoteProcessHandle(handle),
                                                                   m_pClientPool(pool)
{
}

CNativeClient::~CNativeClient()
{
}

CNativeClient *CNativeClient::create(const SESSIONID id,
                                     CMessageQueue *main, HANDLE handle)
{
    CNativeClient *client = new CNativeClient(id, NULL, main, handle);
    ASSERT(client->initialize());
    return client;
}

void CNativeClient::IMEOnClientActive(bool active)
{
    std::shared_ptr<Message> msg = Message::create();
    msg->cookie.setCMD(MIME_CLIENT_ACTIVE);
    msg->cookie.write(m_szId);
    msg->cookie.write(active);
    m_pMainQueue->postMessage(msg);
}

void CNativeClient::IMEOnClientSelect(bool select)
{
    std::shared_ptr<Message> msg = Message::create();
    msg->cookie.setCMD(MIME_CLIENT_SELECT);
    msg->cookie.write(m_szId);
    msg->cookie.write(select);
    m_pMainQueue->postMessage(msg);
}

void CNativeClient::terminate()
{
    if (m_pClientPool)
        m_pClientPool->recycle(this);
    else
        destroy();
}

void CNativeClient::destroy()
{
    CIPCThread::terminate();
    delete this;
}

void CNativeClient::OnDataReceived(PACKET &data, PACKET *reply)
{
    switch (data.getCMD())
    {
    case MIME_CLIENT_ACTIVE:
    {
        bool active;
        data.read(active);
        IMEOnClientActive(active);
        break;
    }
    case MIME_CLIENT_SELECT:
    {
        bool select;
        data.read(select);
        IMEOnClientSelect(select);
        break;
    }
    case MIME_STATUS_BAR_ERROR:
    {
        std::wstring error;
        data.read(error);
        IMEOnError(error);
        break;
    }
    default:
        CIPCThread::OnDataReceived(data, reply);
    }
}
void CNativeClient::OnIOTimeout()
{
    static int timeout_count = 0;
    timeout_count++;
    if (timeout_count > 10)
    {
        PACKET p;
        p.setCMD(MIME_CHECK_PROCESS);
        p.write(m_szId);
        m_pMainQueue->postMessage(&p);
        timeout_count = 0;
    }
    Log::D(L"NativeClient IO timeout.(id: %s)(%s)\n", m_szId.c_str(),
           getProcessNameByHandle(m_hRemoteProcessHandle).c_str());
}

void CNativeClient::OnExitThread()
{
    if (m_hRemoteProcessHandle)
        CloseHandle(m_hRemoteProcessHandle);
}

void CNativeClient::IMEOnNewWords(std::wstring words)
{
    PACKET p;
    p.setCMD(MIME_NEW_CHAR);
    p.write(words);
    send(p, NULL);
}

void CNativeClient::IMEOnError(std::wstring &error)
{
    std::shared_ptr<Message> msg = Message::create();
    msg->cookie.setCMD(MIME_STATUS_BAR_ERROR);
    msg->cookie.write(error);
    m_pMainQueue->postMessage(msg);
}

void CNativeClient::IMEOnDeviceConnected(eConnType type)
{
    PACKET p;
    p.setCMD(MIME_DEVICE_CONNECTED);
    p.write(type);
    send(p, NULL);
}

void CNativeClient::IMEOnDeviceConnectionFail()
{
    PACKET p;
    p.setCMD(MIME_DEVICE_OFFLINE);
    send(p, NULL);
}

void CNativeClient::IMEOnDeviceConnecting()
{
    PACKET p;
    p.setCMD(MIME_DEVICE_CONNECTING);
    send(p, NULL);
}

void CNativeClient::IMEOnDeviceNone()
{
    PACKET p;
    p.setCMD(MIME_DEVICE_NONE);
    send(p, NULL);
}

} // namespace DroidPad