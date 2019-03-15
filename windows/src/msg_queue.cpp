#include "common.h"
#include "msg_queue.h"
#include "SecurityAttribute.h"
#include "util.h"
#include "auto_lock.h"

namespace DroidPad
{


void Message::becomeSync()
{
    if (m_hSyncEvent == NULL)
        m_hSyncEvent = CreateEvent(lpSA, false, false, NULL);
}

void Message::wait()
{
    if (m_hSyncEvent != NULL)
        WaitForSingleObject(m_hSyncEvent, INFINITE);
}

void Message::signal()
{
    if (m_hSyncEvent != NULL)
        SetEvent(m_hSyncEvent);
}

Message::~Message()
{
    if (m_hSyncEvent != NULL)
        CloseHandle(m_hSyncEvent);
    m_hSyncEvent = NULL;
}

std::shared_ptr<Message> Message::create()
{
    Message *msg = new Message;
    return std::shared_ptr<Message>(msg, Message::Deleter());
}

//////////////////////////////////////////////////////////////////////

CMessageQueue::CMessageQueue(MessageHandler *hd) : m_hMsgHandler(hd),
                                                 m_hWakeUpEvent(NULL),
                                                 m_bPendingExit(false),
                                                 m_bSuspended(false)
{
	InitializeCriticalSection(&cs);
    ASSERT((m_hWakeUpEvent = CreateEvent(lpSA, false, false, NULL)) != NULL);
}

CMessageQueue::~CMessageQueue()
{
    if (m_hWakeUpEvent != NULL)
        CloseHandle(m_hWakeUpEvent);
	DeleteCriticalSection(&cs);
}

void CMessageQueue::loop()
{
	while(true)
	{
		{
			CAutoLock _l(&cs);
			if(m_bPendingExit)
				return;
		}
		processMessage();
	}
}

void CMessageQueue::quit()
{
	CAutoLock _l(&cs);
    m_bPendingExit = true;
    if (m_bSuspended)
        SetEvent(m_hWakeUpEvent);
}

void CMessageQueue::sendMessage(std::shared_ptr<Message> msg, u_long delay)
{
    msg->becomeSync();
    postMessageAtFront(msg, delay);
    msg->wait();
}

void CMessageQueue::postMessage(PACKET *packet, u_long delay)
{

    std::shared_ptr<Message> msg = Message::create();
    msg->cookie = *packet;
    postMessage(msg, delay);
}

void CMessageQueue::postMessage(std::shared_ptr<Message> msg, u_long delay)
{
    msg->when = GetTickCount() + delay;
	CAutoLock _l(&cs);
	msgQueue.push_back(msg);
	std::stable_sort(msgQueue.begin(), msgQueue.end(), Message::MsgComp());
	if (m_bSuspended)
		SetEvent(m_hWakeUpEvent);
}

void CMessageQueue::postMessageAtFront(std::shared_ptr<Message> msg, u_long delay)
{
    msg->when = GetTickCount() + delay;
	{
		CAutoLock _l(&cs);
		msgQueue.push_front(msg);
		std::stable_sort(msgQueue.begin(), msgQueue.end(), Message::MsgComp());
		if (m_bSuspended)
			SetEvent(m_hWakeUpEvent);
	}
}

void CMessageQueue::unLockAndWaitRelative(u_long time)
{
	m_bSuspended = true;
    unLockSC();
    WaitForSingleObject(m_hWakeUpEvent, time);
	{
		CAutoLock _l(&cs);
		m_bSuspended = false;
	}
}

void CMessageQueue::processMessage()
{
    lockSC();
    if (msgQueue.empty())
    {
        unLockAndWaitRelative(INFINITE);
    }
    else
    {
        std::shared_ptr<Message> msg = msgQueue.front();
        u_long now = GetTickCount();
        if (msg->when > now)
            unLockAndWaitRelative(msg->when - now);
        else
        {
            msgQueue.pop_front();
            unLockSC();
            m_hMsgHandler->OnHandleMessage(msg);
            //signal if the message is sync
            if(msg->m_hSyncEvent)
                msg->signal();
        }
    }
}

}