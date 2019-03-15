#include "common.h"
#include "util.h"
#include <TlHelp32.h>

using namespace DroidPad;

#define SERVER_KEY L"Software\\DroidPadServer"
#define SERVER_BIN_VALUE L"BIN"
#define SERVER_ROOT_VALUE L"ROOT"

static HANDLE marker = NULL;

std::wstring genRandomID()
{
	wchar_t buffer[GUID_LEN] = {0};
	GUID guid;

	CoCreateGuid(&guid);
	_snwprintf_s(buffer, GUID_LEN - 1,
				 L"%08X%04X%04X",
				 guid.Data1, guid.Data2, guid.Data3);
	return buffer;
}

bool isProcAlive(HANDLE remote)
{
	DWORD ret = WaitForSingleObject(remote, 500);
	return (ret == WAIT_OBJECT_0) ? false : true;
}

eSTATUS markServerStarted()
{
	marker = CreateMutex(NULL, false, L"ime_server_marker");
	return marker == NULL ? STATUS_UNKNOWN_ERROR : STATUS_NO_ERROR;
}
bool isServerStarted()
{
	HANDLE mutex = OpenMutex(MUTEX_ALL_ACCESS, false, L"ime_server_marker");
	if (mutex != NULL)
	{
		CloseHandle(mutex);
		return true;
	}
	return false;
}
void unMarkServerStarted()
{
	ReleaseMutex(marker);
	marker = NULL;
}

eSTATUS getRootPath(LPWCH path, DWORD size)
{
// #ifdef _DEBUG
// 	wcsncpy_s(path, size, L"C:\\", 3);
// 	return STATUS_NO_ERROR;
// #endif
	HKEY hKey;
	DWORD dwType = REG_SZ;
	if (RegOpenKeyEx(HKEY_CURRENT_USER, SERVER_KEY, 0, KEY_READ, &hKey) != ERROR_SUCCESS)
	{
		PRINT_FUNC_ERROR("Util::getRootPath");
		return STATUS_REG_ERROR;
	}
	else
	{
		if (RegQueryValueEx(hKey, SERVER_ROOT_VALUE, 0, &dwType, (LPBYTE)path, &size) != ERROR_SUCCESS)
		{
			RegCloseKey(hKey);
			PRINT_FUNC_ERROR("Util::getRootPath");
			return STATUS_REG_ERROR;
		}
		RegCloseKey(hKey);
	}
	return STATUS_NO_ERROR;
}

eSTATUS getServerBin(LPWCH str, DWORD size)
{
// #ifdef _DEBUG
// 	WCHAR *wc = L"WritingPadServer.exe";
// 	wcsncpy_s(str, size, wc, wcslen(wc));
// 	return STATUS_NO_ERROR;
// #endif
	HKEY hKey;
	DWORD dwType = REG_SZ;
	if (RegOpenKeyEx(HKEY_CURRENT_USER, SERVER_KEY, 0, KEY_READ, &hKey) != ERROR_SUCCESS)
	{
		PRINT_FUNC_ERROR("Util::getServerBin");
		return STATUS_REG_ERROR;
	}
	else
	{
		if (RegQueryValueEx(hKey, SERVER_BIN_VALUE, 0, &dwType, (LPBYTE)str, &size) != ERROR_SUCCESS)
		{
			RegCloseKey(hKey);
			PRINT_FUNC_ERROR("Util::getServerBin");
			return STATUS_REG_ERROR;
		}
		RegCloseKey(hKey);
	}
	return STATUS_NO_ERROR;
}

eSTATUS launchServer()
{
	WCHAR exe_path[MAX_PATH] = {'\0'};
	DWORD path_size = sizeof(exe_path);
	WCHAR root[MAX_PATH] = {'\0'};
	WCHAR bin[MAX_PATH] = {'\0'};
	eSTATUS ret;
	if ((ret = getRootPath(root, MAX_PATH - 1)) != STATUS_NO_ERROR)
		return ret;
	if ((ret = getServerBin(bin, MAX_PATH - 1)) != STATUS_NO_ERROR)
		return ret;
	_snwprintf_s(exe_path, MAX_PATH, L"%s\\%s", root, bin);

	SHELLEXECUTEINFO sei;
	sei.cbSize = sizeof(SHELLEXECUTEINFO);
	sei.fMask = SEE_MASK_NOCLOSEPROCESS;
	sei.hwnd = NULL;
	sei.lpVerb = NULL;
	sei.lpFile = exe_path;
	sei.lpParameters = NULL;
	sei.lpDirectory = NULL;
	sei.nShow = SW_HIDE;
	sei.hInstApp = NULL;

	if (ShellExecuteEx(&sei))
	{
		//Wait for the process within timeout 1s.
		//If the process returns, then it has been dead.
		DWORD ret = WaitForSingleObject(sei.hProcess, 1000);
		if (ret == WAIT_OBJECT_0)
			return STATUS_UNKNOWN_ERROR;
		else
		{
			CloseHandle(sei.hProcess);
			return STATUS_NO_ERROR;
		}
	}
	return STATUS_UNKNOWN_ERROR;
}

SIZE getDesktopSize()
{
	RECT rect;
	SystemParametersInfo(SPI_GETWORKAREA, 0, &rect, 0);
	SIZE size;
	size.cx = rect.right - rect.left;
	size.cy = rect.bottom - rect.top;
	return size;
}

std::string wideToANSI(LPCWSTR wc)
{
	char *pChar;
	int buffLen;
	buffLen = WideCharToMultiByte(CP_ACP, 0, wc, -1,
								  NULL, 0, NULL, NULL);
	pChar = new char[buffLen + 1];
	memset((void *)pChar, '\0', sizeof(char) * (buffLen + 1));
	WideCharToMultiByte(CP_ACP, 0, wc, -1,
						pChar, buffLen, NULL, NULL);
	std::string str;
	str = pChar;
	delete[] pChar;
	return str;
}

std::wstring ansiToWide(LPCSTR str)
{
	WCHAR *wpChar;
	int buffLen;
	buffLen = MultiByteToWideChar(CP_ACP, 0, str, -1, NULL, 0);
	buffLen = (buffLen + 1) * sizeof(WCHAR);
	wpChar = new WCHAR[buffLen];
	memset((void *)wpChar, 0, buffLen);
	MultiByteToWideChar(CP_ACP, 0, str, -1, wpChar, buffLen);
	std::wstring wstr;
	wstr = wpChar;
	delete[] wpChar;
	return wstr;
}

void dumpMem(const char *mem, int bytes)
{
	if (bytes <= 0)
		return;
	for (int i = 0; i < bytes; i++)
		Log::D("%d ", *(mem + i));
}

std::wstring getProcessNameByHandle(HANDLE handle)
{
	std::wstring ret = L"";
	DWORD dwPid = GetProcessId(handle);
	if (dwPid == 0)
		return L"";
	HANDLE snapHandle = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	if (snapHandle == (HANDLE)-1)
		return L"";
	PROCESSENTRY32 procinfo;
	procinfo.dwSize = sizeof(PROCESSENTRY32);
	BOOL more = ::Process32First(snapHandle, &procinfo);
	while (more)
	{
		if (procinfo.th32ProcessID == dwPid)
		{
			ret = procinfo.szExeFile;
			CloseHandle(snapHandle);
			return ret;
		}
		more = Process32Next(snapHandle, &procinfo);
	}
	CloseHandle(snapHandle);
	return ret;
}