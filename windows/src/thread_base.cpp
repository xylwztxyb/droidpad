#include "thread_base.h"
#include "SecurityAttribute.h"
#include "auto_lock.h"
#include "util.h"

namespace DroidPad
{


CThreadBase::CThreadBase() : m_hThreadHandle(NULL),
						   m_bInited(false),
						   m_bPendingExit(false),
						   m_bPendingSuspend(false),
						   m_bAlive(false),
						   m_bSuspended(false),
						   m_hWakeUpEvent(NULL)
{
	InitializeCriticalSection(&m_cs);
}

CThreadBase::~CThreadBase()
{
	DeleteCriticalSection(&m_cs);
}

bool CThreadBase::OnCreateThread()
{
	return true;
}

void CThreadBase::OnStartThread()
{
}

void CThreadBase::OnExitThread()
{
}

void CThreadBase::OnDestroyThread()
{
}

bool CThreadBase::initialize()
{
	if (m_bInited)
		return true;
	m_hWakeUpEvent = CreateEvent(lpSA, false, false, NULL);
	if (m_hWakeUpEvent == NULL)
	{
		PRINT_FUNC_ERROR("ThreadBase::initialize");
		return false;
	}
	m_hThreadHandle = (HANDLE)_beginthreadex(NULL, 0, _threadEntry, this, CREATE_SUSPENDED, &m_iThreadId);
	if (m_hThreadHandle == NULL)
	{
		PRINT_FUNC_ERROR("ThreadBase::initialize");
		return false;
	}
	if (!OnCreateThread())
		return false;
	return m_bInited = true;
}

void CThreadBase::terminate()
{
	if (!m_bInited)
		return;
	OnExitThread();
	{
		CAutoLock _l(&m_cs);
		m_bPendingExit = true;
		if (m_bSuspended)
		{
			m_bPendingSuspend = false;
			SetEvent(m_hWakeUpEvent);
		}
	}
	WaitForSingleObject(m_hThreadHandle, INFINITE);
	CloseHandle(m_hThreadHandle);
	m_hThreadHandle = NULL;
	CloseHandle(m_hWakeUpEvent);
	m_hWakeUpEvent = NULL;
	m_bAlive = false;
	m_bInited = false;
}

bool CThreadBase::resumeThread()
{
	if (!m_bInited)
		return false;
	if (!m_bAlive)
	{
		DWORD ret = ResumeThread(m_hThreadHandle);
		if (ret == -1 || ret > 1)
			return false;
		else
		{
			OnStartThread();
			return m_bAlive = true;
		}
	}
	{
		CAutoLock _l(&m_cs);
		if (m_bSuspended)
		{
			m_bPendingSuspend = false;
			SetEvent(m_hWakeUpEvent);
		}
	}
	return true;
}

void CThreadBase::suspendThread()
{
	if (!m_bInited || !m_bAlive)
		return;
	{
		CAutoLock _l(&m_cs);
		if (!m_bSuspended)
		{
			m_bPendingSuspend = true;
		}
	}
}

void CThreadBase::suspendInner()
{
	while (true)
	{
		{
			CAutoLock _l(&m_cs);
			m_bSuspended = true;
		}
		WaitForSingleObject(m_hWakeUpEvent, INFINITE);
		{
			CAutoLock _l(&m_cs);
			if (!m_bPendingSuspend)
			{
				m_bSuspended = false;
				break;
			}
		}
	}
}

unsigned int __stdcall CThreadBase::_threadEntry(void *args)
{
	CThreadBase *thiz = (CThreadBase *)args;
	while (true)
	{
		while (true)
		{
			{
				CAutoLock _l(&thiz->m_cs);
				if (!thiz->m_bPendingSuspend)
				{
					thiz->m_bSuspended = false;
					break;
				}
				else
					thiz->m_bSuspended = true;
			}
			WaitForSingleObject(thiz->m_hWakeUpEvent, INFINITE);
		}

		{
			CAutoLock _l(&thiz->m_cs);
			if (thiz->m_bPendingExit)
			{
				thiz->OnDestroyThread();
				_endthreadex(1);
				return 0;
			}
		}

		if (!thiz->threadLoop())
		{
			thiz->OnDestroyThread();
			_endthreadex(1);
			return 0;
		}
	}
}

}