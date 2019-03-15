// dllmain.cpp : 定义 DLL 应用程序的入口点。
#include "common.h"
#include "mime.h"
#include "writing_pad_service.h"

static CWritingPadService *gService = NULL;
static SESSIONID id;

BOOL APIENTRY DllMain(HMODULE hModule,
					  DWORD ul_reason_for_call,
					  LPVOID lpReserved)
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
		registerUIClass(hModule);
		gService = CWritingPadService::getInstance();
		if (!gService)
			return FALSE;
		if (gService->OpenSession(GetCurrentProcess(), &id) != STATUS_NO_ERROR)
			return FALSE;
		if (id.empty())
			return FALSE;
		gRemoteClient = new CRemoteClient(id);
		break;
	case DLL_THREAD_ATTACH:
		break;
	case DLL_THREAD_DETACH:
		break;
	case DLL_PROCESS_DETACH:
		unregisterUIClass(hModule);
		if (gRemoteClient)
		{
			if (gRemoteClient->isAlive())
				gRemoteClient->terminate();
			delete gRemoteClient;
		}
		if (gService)
		{
			if (!id.empty())
				gService->CloseSession(id);
			gService->Destroy();
			gService = NULL;
			id.clear();
		}
		break;
	}
	return TRUE;
}
