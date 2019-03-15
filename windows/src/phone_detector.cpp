#include "phone_detector.h"
#include <dbt.h>
#include <strsafe.h>
#include <sstream>
#include <regex>
#include <set>
#include "util.h"

#define WINDOW_CLASS_NAME L"PhoneDetector"
#define MAX_PIPE_READ_BYTES 4096

GUID usbGUID = {0x25dbce51, 0x6c8f, 0x4a72,
					 0x8a, 0x6d, 0xb5, 0x4c, 0x2b, 0x4f, 0xc8, 0x35};

static std::set<std::string> g_DeviceSerials;

static LRESULT __stdcall WndProc(HWND hwd, UINT message, WPARAM wParam, LPARAM lParam);

static void ErrorHandler(LPTSTR lpszFunction)
// Routine Description:
//     Support routine.
//     Retrieve the system error message for the last-error code
//     and pop a modal alert box with usable info.

// Parameters:
//     lpszFunction - String containing the function name where
//     the error occurred plus any other relevant data you'd
//     like to appear in the output.

// Return Value:
//     None

// Note:
//     This routine is independent of the other windowing routines
//     in this application and can be used in a regular console
//     application without modification.
{

	LPVOID lpMsgBuf;
	LPVOID lpDisplayBuf;
	DWORD dw = GetLastError();

	FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER |
			FORMAT_MESSAGE_FROM_SYSTEM |
			FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL,
		dw,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR)&lpMsgBuf,
		0, NULL);

	// Display the error message and exit the process.

	lpDisplayBuf = (LPVOID)LocalAlloc(LMEM_ZEROINIT,
									  (lstrlen((LPCTSTR)lpMsgBuf) + lstrlen((LPCTSTR)lpszFunction) + 40) * sizeof(TCHAR));
	StringCchPrintf((LPTSTR)lpDisplayBuf,
					LocalSize(lpDisplayBuf) / sizeof(TCHAR),
					TEXT("%s failed with error %d: %s"),
					lpszFunction, dw, lpMsgBuf);
	MessageBox(NULL, (LPCTSTR)lpDisplayBuf, NULL, MB_OK);

	LocalFree(lpMsgBuf);
	LocalFree(lpDisplayBuf);
}

static bool doRegisterDeviceInterfaceToHwnd(
	IN GUID InterfaceClassGuid,
	IN HWND hWnd,
	OUT HDEVNOTIFY *hDeviceNotify)
// Routine Description:
//     Registers an HWND for notification of changes in the device interfaces
//     for the specified interface class GUID.

// Parameters:
//     InterfaceClassGuid - The interface class GUID for the device
//         interfaces.

//     hWnd - Window handle to receive notifications.

//     hDeviceNotify - Receives the device notification handle. On failure,
//         this value is NULL.

// Return Value:
//     If the function succeeds, the return value is TRUE.
//     If the function fails, the return value is FALSE.

// Note:
//     RegisterDeviceNotification also allows a service handle be used,
//     so a similar wrapper function to this one supporting that scenario
//     could be made from this template.
{
	DEV_BROADCAST_DEVICEINTERFACE NotificationFilter;

	ZeroMemory(&NotificationFilter, sizeof(NotificationFilter));
	NotificationFilter.dbcc_size = sizeof(DEV_BROADCAST_DEVICEINTERFACE);
	NotificationFilter.dbcc_devicetype = DBT_DEVTYP_DEVICEINTERFACE;
	NotificationFilter.dbcc_classguid = InterfaceClassGuid;

	*hDeviceNotify = RegisterDeviceNotification(
		hWnd,						// events recipient
		&NotificationFilter,		// type of device
		DEVICE_NOTIFY_WINDOW_HANDLE // type of recipient handle
	);

	if (NULL == *hDeviceNotify)
	{
		ErrorHandler(TEXT("RegisterDeviceNotification"));
		return false;
	}

	return *hDeviceNotify != NULL;
}

static ATOM _regCls(HINSTANCE hInst)
{
	WNDCLASSEX cls;
	cls.cbSize = sizeof(WNDCLASSEX);
	cls.style = CS_HREDRAW | CS_VREDRAW;
	cls.lpfnWndProc = WndProc;
	cls.cbClsExtra = 0;
	cls.cbWndExtra = 0;
	cls.hInstance = hInst;
	cls.hIcon = LoadIcon(NULL, IDI_APPLICATION);
	cls.hIconSm = LoadIcon(NULL, IDI_APPLICATION);
	cls.hCursor = LoadCursor(NULL, IDC_ARROW);
	cls.hbrBackground = (HBRUSH)COLOR_BACKGROUND;
	cls.lpszMenuName = NULL;
	cls.lpszClassName = WINDOW_CLASS_NAME;

	return RegisterClassEx(&cls);
}

static bool _createWindow(HINSTANCE hInst, HWND *hwd)
{
	DWORD dwStyle = WS_OVERLAPPEDWINDOW ^ WS_THICKFRAME; //Disable changing size

	//Create a invisible window
	*hwd = CreateWindowEx(0, WINDOW_CLASS_NAME, NULL, dwStyle,
		0, 0, 0, 0,
		NULL, NULL, hInst, NULL);
	if (!*hwd)
		return false;

	LONG style = GetWindowLong(*hwd, GWL_STYLE);
	if (style & WS_CAPTION)
		style ^= WS_CAPTION;
	SetWindowLong(*hwd, GWL_STYLE, style & ~WS_SYSMENU); //Remove close button
	return true;
}

static bool doAdbCommand(LPCWCHAR cmd, std::string *out)
{
	STARTUPINFO si;
	PROCESS_INFORMATION pi;

	ZeroMemory(&si, sizeof(si));
	si.cb = sizeof(si);
	ZeroMemory(&pi, sizeof(pi));

	WCHAR command[MAX_PATH] = {0};
	wcsncpy_s(command, cmd, wcslen(cmd));

	SECURITY_ATTRIBUTES sa;
	HANDLE hRead, hWrite;
	sa.nLength = sizeof(SECURITY_ATTRIBUTES);
	sa.lpSecurityDescriptor = NULL; 
	sa.bInheritHandle = TRUE;

	if (!CreatePipe(&hRead, &hWrite, &sa, 0))
	{
		ErrorHandler(L"CreatePipe");
		return false;
	}

	si.hStdError = hWrite;
	si.hStdOutput = hWrite;
	si.wShowWindow = SW_HIDE;
	si.dwFlags = STARTF_USESHOWWINDOW | STARTF_USESTDHANDLES;

	// Start the child process.
	if (!CreateProcess(NULL,				// No module name (use command line)
		command, // Command line
		NULL,				// Process handle not inheritable
		NULL,				// Thread handle not inheritable
		TRUE,				// Set handle inheritance to FALSE
		0,					// No creation flags
		NULL,				// Use parent's environment block
		NULL,				// Use parent's starting directory
		&si,					// Pointer to STARTUPINFO structure
		&pi)					// Pointer to PROCESS_INFORMATION structure
		)
	{
		ErrorHandler(L"CreateProcess");
		return false;
	}
	WaitForSingleObject(pi.hProcess, INFINITE);
	CloseHandle(hWrite);

	CHAR buffer[MAX_PIPE_READ_BYTES];
	DWORD bytesRead;

	if (out)
	{
		while (true)
		{
			memset(buffer, 0, sizeof(buffer));
			bytesRead = 0;
			if (ReadFile(hRead, buffer, sizeof(buffer), &bytesRead, NULL) == FALSE)
				break;
			out->append(buffer, bytesRead);
			Sleep(100);
		}
	}

	CloseHandle(hRead);
	CloseHandle(pi.hProcess);
	CloseHandle(pi.hThread);
	return true;
}

static void setupDevice(const std::string &serial)
{
	WCHAR root[MAX_PATH] = {0};
	if(getRootPath(root, MAX_PATH - 1) != STATUS_NO_ERROR)
	{
		PRINT_FUNC_ERROR("CPhoneDetector::setupDevice");
		return;
	}
	const WCHAR *cmd_format = L"%s\\adb.exe -s %s reverse tcp:%d tcp:%d";
	WCHAR command[MAX_PATH] = {0};
	_snwprintf_s(command, MAX_PATH - 1, cmd_format, root, ansiToWide(serial.c_str()).c_str(), ADB_LISTEN_PORT, ADB_LISTEN_PORT);
	doAdbCommand(command, NULL);
}

static void _handleSerial(const std::string &serial)
{
	if (g_DeviceSerials.find(serial) == g_DeviceSerials.end())
		g_DeviceSerials.insert(serial);
	setupDevice(serial);
}

static void _updateSerialList(const std::set<std::string> list)
{
	std::set<std::string>::iterator it;
	for (it = g_DeviceSerials.begin(); it != g_DeviceSerials.end();)
	{
		if (list.find(*it) == list.end())
			it = g_DeviceSerials.erase(it);
		else
			it++;
	}
}

static void _parseResult(std::string &result)
{
	std::set<std::string> serials;
	std::regex line_pattern("\\W*[[:alnum:]]+[\\W]+[Dd][Ee][Vv][Ii][Cc][Ee]\\W*");
	std::regex serial_pattern("[[:alnum:]]+");
	std::istringstream str_buffer(result);
	for (std::string line; std::getline(str_buffer, line, '\n');)
	{
		std::smatch sm;
		if (std::regex_match(line, line_pattern) && std::regex_search(line, sm, serial_pattern))
			serials.insert(sm.str());
	}
	if (!serials.empty())
	{
		for (std::set<std::string>::iterator it = serials.begin();
			it != serials.end(); it++)
			_handleSerial(*it);
		_updateSerialList(serials);
	}
}

static void detectDevice()
{
	WCHAR root[MAX_PATH] = {0};
	if(getRootPath(root, MAX_PATH - 1) != STATUS_NO_ERROR)
	{
		PRINT_FUNC_ERROR("CPhoneDetector::detectDevice");
		return;
	}
	WCHAR command[MAX_PATH] = {0};
	const WCHAR *format = L"%s\\adb.exe devices";
	_snwprintf_s(command, MAX_PATH - 1, format, root);

	//const WCHAR *cmd = L"D:/AndroidSdk/platform-tools/adb.exe devices";
	std::string string_result;
	if (doAdbCommand(command, &string_result) && !string_result.empty())
		_parseResult(string_result);
}

static LRESULT __stdcall WndProc(HWND hwd, UINT message, WPARAM wParam, LPARAM lParam)
{
	static HDEVNOTIFY _hDevNotify;
	switch (message)
	{
	case WM_CREATE:
		doRegisterDeviceInterfaceToHwnd(usbGUID, hwd, &_hDevNotify);
		break;
	case WM_MOUSEACTIVATE:
		return MA_NOACTIVATE; //Disable focus
	case WM_DEVICECHANGE:
		{
			switch (wParam)
			{
			case DBT_DEVNODES_CHANGED:
				Sleep(2000);
				detectDevice();
				break;
			}
		}
		break;
	case WM_DESTROY:
		UnregisterDeviceNotification(_hDevNotify);
		PostQuitMessage(0);
		break;
	default:
		// Send all other messages on to the default windows handler.
		return DefWindowProc(hwd, message, wParam, lParam);
	}
	return S_OK;
}

/////////////////////////////////////

namespace DroidPad
{


CPhoneDetector::CPhoneDetector() : m_hWnd(NULL)
{
}

CPhoneDetector::~CPhoneDetector()
{
}

void CPhoneDetector::showMessage(std::wstring &msg)
{
	MessageBox(m_hWnd, msg.c_str(), L"Error", MB_OK);
}

void CPhoneDetector::OnExitThread()
{
	SendMessage(m_hWnd, WM_DESTROY, NULL, NULL);
}

bool CPhoneDetector::threadLoop()
{
	HINSTANCE hIns = GetModuleHandle(NULL);
	if (!hIns)
		return false;
	_regCls(hIns);
	if (!_createWindow(hIns, &m_hWnd))
		return false;

	MSG msg;
	BOOL bRet;

	while ((bRet = GetMessage(&msg, NULL, 0, 0)) != 0)
	{
		if (bRet == -1)
		{
			return false;
		}
		else
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}
	return false;
}

}



