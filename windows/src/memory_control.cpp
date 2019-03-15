#include "memory_control.h"
#include "SecurityAttribute.h"
#include "util.h"

namespace DroidPad
{

SMemCtrlBlk::SMemCtrlBlk(LPCWSTR szId) : m_szIdentify(NULL),
                                         m_hMutex(NULL),
                                         m_hEvent_Read_Done(NULL),
                                         m_hEvent_Write_Done(NULL)
{
    ASSERT(szId != NULL);
    size_t str_len = wcslen(szId);
    m_szIdentify = new WCHAR[str_len + 1];
    memset(m_szIdentify, 0, (str_len + 1) * sizeof(WCHAR));
    wcsncpy_s(m_szIdentify, str_len + 1, szId, str_len);
}

SMemCtrlBlk::~SMemCtrlBlk()
{
    if (m_hMutex)
        CloseHandle(m_hMutex);
    if (m_hEvent_Read_Done)
        CloseHandle(m_hEvent_Read_Done);
    if (m_hEvent_Write_Done)
        CloseHandle(m_hEvent_Write_Done);
    if (m_szIdentify)
        delete[] m_szIdentify;

    m_hMutex = NULL;
    m_hEvent_Read_Done = NULL;
    m_hEvent_Write_Done = NULL;
    m_szIdentify = NULL;
}

eSTATUS SMemCtrlBlk::init(bool bCreate)
{
    if (bCreate)
    {
        m_hMutex = CreateMutex(lpSA, false, m_szIdentify);
        if (m_hMutex == NULL)
        {
            PRINT_FUNC_ERROR("tagMemCntrlBlck::init");
            return STATUS_UNKNOWN_ERROR;
        }
        m_hEvent_Read_Done = CreateEvent(lpSA, false, false, WStrCatHelper(m_szIdentify, L"_ReadDone").szwResult);
        if (m_hEvent_Read_Done == NULL)
        {
            PRINT_FUNC_ERROR("tagMemCntrlBlck::init");
            return STATUS_UNKNOWN_ERROR;
        }

        m_hEvent_Write_Done = CreateEvent(lpSA, false, false, WStrCatHelper(m_szIdentify, L"_WriteDone").szwResult);
        if (m_hEvent_Write_Done == NULL)
        {
            PRINT_FUNC_ERROR("tagMemCntrlBlck::init");
            return STATUS_UNKNOWN_ERROR;
        }
    }
    else
    {
        m_hMutex = OpenMutex(MUTEX_ALL_ACCESS, false, m_szIdentify);
        if (m_hMutex == NULL)
        {
            PRINT_FUNC_ERROR("tagMemCntrlBlck::init");
            return STATUS_UNKNOWN_ERROR;
        }

        m_hEvent_Read_Done = OpenEvent(EVENT_ALL_ACCESS, true, WStrCatHelper(m_szIdentify, L"_ReadDone").szwResult);
        if (m_hEvent_Read_Done == NULL)
        {
            PRINT_FUNC_ERROR("tagMemCntrlBlck::init");
            return STATUS_UNKNOWN_ERROR;
        }

        m_hEvent_Write_Done = OpenEvent(EVENT_ALL_ACCESS, true, WStrCatHelper(m_szIdentify, L"_WriteDone").szwResult);
        if (m_hEvent_Write_Done == NULL)
        {
            PRINT_FUNC_ERROR("tagMemCntrlBlck::init");
            return STATUS_UNKNOWN_ERROR;
        }
    }

    return STATUS_NO_ERROR;
}

void SMemCtrlBlk::cleanEventState()
{
    ResetEvent(m_hEvent_Read_Done);
    ResetEvent(m_hEvent_Write_Done);
}

DWORD SMemCtrlBlk::wait_read_done(u_long ulTimeout)
{
    return WaitForSingleObject(m_hEvent_Read_Done, ulTimeout);
}

void SMemCtrlBlk::signal_write_done()
{
    SetEvent(m_hEvent_Write_Done);
}

DWORD SMemCtrlBlk::wait_write_done(u_long ulTimeout)
{
    return WaitForSingleObject(m_hEvent_Write_Done, ulTimeout);
}

void SMemCtrlBlk::signal_read_done()
{
    SetEvent(m_hEvent_Read_Done);
}

void SMemCtrlBlk::_lock()
{
    WaitForSingleObject(m_hMutex, INFINITE);
}

void SMemCtrlBlk::_release()
{
    ReleaseMutex(m_hMutex);
}

} // namespace DroidPad