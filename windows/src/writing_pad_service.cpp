#include "common.h"
#include "writing_pad_service.h"
#include "util.h"

DroidPad::CWritingPadService *gService = NULL;

namespace DroidPad
{


CWritingPadService::CWritingPadService() : m_pChannel(NULL)
{
	m_pChannel = new CChannelBase(WRITING_PAD_SERVICE_NAME, true);
}

CWritingPadService::~CWritingPadService()
{
	if (m_pChannel != NULL)
		delete m_pChannel;
}

eSTATUS CWritingPadService::init()
{
	return m_pChannel->init();
}

CWritingPadService *CWritingPadService::getInstance()
{
	if (gService == NULL)
		gService = new CWritingPadService();

	do
	{
		if (gService->init() != STATUS_NO_ERROR)
		{
			if (launchServer() == STATUS_NO_ERROR)
				continue;
			else
				return NULL;
		}
		else
			return gService;
	} while (true);
}

void CWritingPadService::Destroy()
{
	gService = NULL;
	delete this;
}

u_long CWritingPadService::getServerPid()
{
	PACKET p, reply;
	p.setCMD(MIME_GET_SERVER_PID);
	if (m_pChannel->send(p, &reply) != STATUS_NO_ERROR)
		Log::D("Warning, channel send timeout, may be error");
	u_long pid;
	reply.read(pid);
	return pid;
}

eSTATUS CWritingPadService::OpenSession(HANDLE handle, SESSIONID *out)
{
	HANDLE real_handle = handle;
	if (real_handle == INVALID_HANDLE_VALUE)
	{
		HANDLE server_handle = OpenProcess(PROCESS_DUP_HANDLE, FALSE, getServerPid());
		if (!server_handle)
			return STATUS_UNKNOWN_ERROR;
		BOOL ret = DuplicateHandle(GetCurrentProcess(), handle,
								   server_handle, &real_handle, 0, FALSE, DUPLICATE_SAME_ACCESS);
		CloseHandle(server_handle);
		if (!ret)
			return STATUS_UNKNOWN_ERROR;
	}

	PACKET p, reply;
	p.setCMD(MIME_OPEN_SESSION);
	p.write(real_handle);
	if (m_pChannel->send(p, &reply) != STATUS_NO_ERROR)
		Log::D("Warning, channel send timeout, may be error");
	reply.read(*out);
	return STATUS_NO_ERROR;
}

void CWritingPadService::CloseSession(SESSIONID id)
{
	PACKET p;
	p.setCMD(MIME_CLOSE_SESSION);
	p.write(id);
	m_pChannel->send(p, NULL);
}

}