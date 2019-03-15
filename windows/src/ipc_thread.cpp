#include "ipc_thread.h"
#include "channel.h"
#include "util.h"

namespace DroidPad
{

CIPCThread::CIPCThread(LPCWSTR szId, bool bClient) : m_pChannel(NULL),
                                                     m_bClient(bClient)
{
    m_pChannel = new CChannelBase(szId, bClient);
    ASSERT(m_pChannel->init() == STATUS_NO_ERROR);
    m_pChannel->registerDataProc(this);
}

CIPCThread::~CIPCThread()
{
    if (m_pChannel != NULL)
        delete m_pChannel;
    m_pChannel = NULL;
}

void CIPCThread::OnDataReceived(PACKET &data, PACKET *reply)
{
    switch (data.getCMD())
    {
    case MIME_EXIT_NOW:
        terminate();
        break;
    }
}

void CIPCThread::OnIOTimeout()
{
}

eSTATUS CIPCThread::send(const PACKET data, PACKET *reply)
{
    return m_pChannel->send(data, reply);
}

bool CIPCThread::threadLoop()
{
    if (m_pChannel->recv() == STATUS_IO_TIMEOUT)
        OnIOTimeout();
    return true;
}

} // namespace DroidPad