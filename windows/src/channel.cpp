#include "channel.h"
#include "SecurityAttribute.h"
#include "util.h"

namespace DroidPad
{

CChannelBase::CChannelBase(LPCWSTR szId, bool bClient) : m_szIdentify(NULL),
                                                         m_hMapFile(NULL),
                                                         m_lpMemBase(NULL),
                                                         m_CtrlBlk_Read(NULL),
                                                         m_CtrlBlk_Write(NULL),
                                                         m_bClient(bClient),
                                                         m_fnCallback(NULL)
{
    ASSERT(szId != NULL);
    size_t str_len = wcslen(szId);
    m_szIdentify = new WCHAR[str_len + 1];
    memset(m_szIdentify, 0, (str_len + 1) * sizeof(WCHAR));
    wcsncpy_s(m_szIdentify, str_len + 1, szId, str_len);
}

CChannelBase::~CChannelBase()
{
    if (m_lpMemBase)
        UnmapViewOfFile(m_lpMemBase);
    if (m_hMapFile)
        CloseHandle(m_hMapFile);
    if (m_CtrlBlk_Read)
        delete m_CtrlBlk_Read;
    if (m_CtrlBlk_Write)
        delete m_CtrlBlk_Write;
    if (m_szIdentify)
        delete[] m_szIdentify;

    m_hMapFile = NULL;
    m_lpMemBase = NULL;
    m_CtrlBlk_Read = NULL;
    m_CtrlBlk_Write = NULL;
    m_szIdentify = NULL;
}

eSTATUS CChannelBase::init()
{
    if (!m_bClient)
    {
        m_hMapFile = CreateFileMapping(INVALID_HANDLE_VALUE, lpSA, PAGE_READWRITE, 0, CHANNEL_MEM_MAX, m_szIdentify);
        if (m_hMapFile == NULL)
        {
            PRINT_FUNC_ERROR("ChannelBase::init");
            return STATUS_MEM_ERROR;
        }
    }
    else
    {
        m_hMapFile = OpenFileMapping(FILE_MAP_ALL_ACCESS, false, m_szIdentify);
        if (m_hMapFile == NULL)
        {
            PRINT_FUNC_ERROR("ChannelBase::init");
            return STATUS_MEM_ERROR;
        }
    }

    m_lpMemBase = MapViewOfFile(m_hMapFile, FILE_MAP_READ | FILE_MAP_WRITE, 0, 0, 0);
    if (m_lpMemBase == NULL)
    {
        PRINT_FUNC_ERROR("ChannelBase::init");
        return STATUS_MEM_ERROR;
    }

    if (!m_bClient)
    {
        //when create the channel in server, we let cb_in -> in, cb_out -> out
        m_CtrlBlk_Read = new SMemCtrlBlk(WStrCatHelper(m_szIdentify, L"_in").szwResult);
        if (m_CtrlBlk_Read->init(true) != STATUS_NO_ERROR)
        {
            PRINT_FUNC_ERROR("ChannelBase::init");
            return STATUS_UNKNOWN_ERROR;
        }

        m_CtrlBlk_Write = new SMemCtrlBlk(WStrCatHelper(m_szIdentify, L"_out").szwResult);
        if (m_CtrlBlk_Write->init(true) != STATUS_NO_ERROR)
        {
            PRINT_FUNC_ERROR("ChannelBase::init");
            return STATUS_UNKNOWN_ERROR;
        }
    }
    else
    {
        //when open the channel in client, we let cb_in -> out, cb_out -> in
        //because the client reads the server's out
        m_CtrlBlk_Read = new SMemCtrlBlk(WStrCatHelper(m_szIdentify, L"_out").szwResult);
        if (m_CtrlBlk_Read->init(false) != STATUS_NO_ERROR)
        {
            PRINT_FUNC_ERROR("ChannelBase::init");
            return STATUS_UNKNOWN_ERROR;
        }

        m_CtrlBlk_Write = new SMemCtrlBlk(WStrCatHelper(m_szIdentify, L"_in").szwResult);
        if (m_CtrlBlk_Write->init(false) != STATUS_NO_ERROR)
        {
            PRINT_FUNC_ERROR("ChannelBase::init");
            return STATUS_UNKNOWN_ERROR;
        }
    }
    return STATUS_NO_ERROR;
}

void CChannelBase::getIdentify(LPWSTR id, size_t len) const
{
    if (len > 0)
        wcsncpy_s(id, len, m_szIdentify, wcslen(m_szIdentify));
}

eSTATUS CChannelBase::send(const PACKET &data, PACKET *reply)
{
    m_CtrlBlk_Write->_lock();           //lock the shared memory
    m_CtrlBlk_Write->cleanEventState(); //clean the previous state
    //The mem address is different when send by client or server
    char *pmem = m_bClient ? (char *)m_lpMemBase : (char *)m_lpMemBase + CHANNEL_MEM_OFFSET;
    memset(pmem, 0, sizeof(PACKET));
    //copy data to mem
    new (pmem) PACKET(data);
    m_CtrlBlk_Write->signal_write_done(); //notify the remote
    m_CtrlBlk_Write->_release();          //release the memory
    //wait for the remote's response
    DWORD ret = m_CtrlBlk_Write->wait_read_done(CHANNEL_SEND_TIMEOUT);
    if (ret == WAIT_OBJECT_0)
    {
        if (reply != NULL)
            //copy the data from mem
            *reply = *(PACKET *)pmem;
    }
    else
    {
        return STATUS_IO_TIMEOUT;
    }
    return STATUS_NO_ERROR;
}

eSTATUS CChannelBase::recv()
{
    DWORD ret;
    ret = m_CtrlBlk_Read->wait_write_done(CHANNEL_RECV_TIMEOUT);
    switch (ret)
    {
    case WAIT_OBJECT_0:
    {
        m_CtrlBlk_Read->_lock();
        //The mem address is different when recv in client and server
        char *pmem = m_bClient ? (char *)m_lpMemBase + CHANNEL_MEM_OFFSET : (char *)m_lpMemBase;
        //copy data from mem
        PACKET data(*(PACKET *)pmem);
        PACKET reply;
        if (m_fnCallback != NULL)
            m_fnCallback->OnDataReceived(data, &reply);

        memset(pmem, 0, sizeof(PACKET));
        //copy the reply data to mem
        new (pmem) PACKET(reply);

        m_CtrlBlk_Read->signal_read_done();
        m_CtrlBlk_Read->_release();
        return STATUS_NO_ERROR;
    }
    break;
    case WAIT_TIMEOUT:
        return STATUS_IO_TIMEOUT;
    default:
        PRINT_FUNC_ERROR("ChannelBase::recv");
        return STATUS_UNKNOWN_ERROR;
    }
}

} // namespace DroidPad