#ifndef __UTIL_H__
#define __UTIL_H__

#include "common.h"

using namespace DroidPad;

#define ASSERT(condition) \
	if (!condition)       \
		abort();

#define PRINT_FUNC_ERROR(FUNC)                                                        \
	do                                                                                \
	{                                                                                 \
		const CHAR *format = "Function:%s failed, with error code(%d) in line(%d)\n"; \
		CHAR info[1024] = {0};                                                        \
		_snprintf_s(info, sizeof(info) - 1, format, FUNC, GetLastError(), __LINE__);  \
		Log::D(info);                                                                 \
	} while (0);

#define PRINT_FUNC_ERROR_WSA(FUNC)                                                        \
	do                                                                                    \
	{                                                                                     \
		const CHAR *format = "Function:%s failed, with wsa error code(%d) in line(%d)\n"; \
		CHAR info[1024] = {0};                                                            \
		_snprintf_s(info, sizeof(info) - 1, format, FUNC, WSAGetLastError(), __LINE__);   \
		Log::D(info);                                                                     \
	} while (0);

struct Log
{
	static void D(const char *format, ...)
	{
		char buff[1024];
		va_list _args;
		va_start(_args, format);
		_vsnprintf_s(buff, sizeof(buff), format, _args);
		printf("%s", buff);
		va_end(_args);
	}

	static void D(const wchar_t *format, ...)
	{
		wchar_t buff[1024];
		va_list _args;
		va_start(_args, format);
		_vsnwprintf_s(buff, sizeof(buff), format, _args);
		wprintf(L"%s", buff);
		va_end(_args);
	}
};

struct StrCatHelper
{
	StrCatHelper(LPCSTR s1, LPCSTR s2):szResult(NULL)
	{
		size_t s1_len = strlen(s1);
		size_t s2_len = strlen(s2);
		size_t result_len = s1_len + s2_len + 1;
		szResult = new CHAR[result_len];
		memset(szResult, 0, (result_len) * sizeof(CHAR));
		strncpy_s(szResult, result_len, s1, s1_len);
		strncpy_s(szResult + s1_len, result_len - s1_len, s2, s2_len);
	}
	~StrCatHelper()
	{
		if(szResult)
			delete[] szResult;
		szResult = NULL;
	}
	LPSTR szResult;
};

struct WStrCatHelper
{
	WStrCatHelper(LPCWSTR s1, LPCWSTR s2):szwResult(NULL)
	{
		size_t s1_len = wcslen(s1);
		size_t s2_len = wcslen(s2);
		size_t result_len = s1_len + s2_len + 1;
		szwResult = new WCHAR[result_len];
		memset(szwResult, 0, result_len * sizeof(WCHAR));
		wcsncpy_s(szwResult, result_len, s1, s1_len);
		wcsncpy_s(szwResult + s1_len, result_len - s1_len, s2, s2_len);
	}
	~WStrCatHelper()
	{
		if(szwResult)
			delete[] szwResult;
		szwResult = NULL;
	}
	LPWSTR szwResult;
};

std::wstring genRandomID();
bool isProcAlive(HANDLE remote);
eSTATUS markServerStarted();
bool isServerStarted();
void unMarkServerStarted();
eSTATUS launchServer();
eSTATUS getRootPath(LPWCH path, DWORD size);
SIZE getDesktopSize();
std::string wideToANSI(LPCWCH wc);
std::wstring ansiToWide(LPCSTR str);
void dumpMem(const char *mem, int bytes);
std::wstring getProcessNameByHandle(HANDLE nlHandle);

#endif