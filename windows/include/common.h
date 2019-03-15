#ifndef _COMMON_H_
#define _COMMON_H_

#include <WinSock2.h>
#include <GdiPlus.h>
#include <string>
#include <stdio.h>
#include <stdarg.h>
#include <memory>
#include <process.h>
#include <assert.h>
#include <map>
#include <deque>
#include <algorithm>
#include <Sddl.h>
#include <aclapi.h>
#include <ObjBase.h>
#include <WinReg.h>
#include <ShellAPI.h>
#include <WinUser.h>

namespace DroidPad
{

////////////////////////////////////////////////////
#define PACKET_BUFF_MAX 1024
#define WRITING_PAD_SERVICE_NAME L"B0A7F6EA-5097-45E4-82D1-38CF11A85252"
#define IME_STATUS_BAR_WIDTH 200
#define IME_STATUS_BAR_HEIGHT 80
#define NATIVE_CLIENT_POOL_MAX 50

#define ADB_LISTEN_PORT 5288
#define WIFI_LISTEN_PORT 59288
///////////////////////////////////////////////////
#define GUID_LEN 17
typedef std::wstring SESSIONID;
///////////////////////////////////////////////////
enum eSTATUS
{
    STATUS_NO_ERROR,
    STSTUS_REMOTE_ERROR,
    STATUS_MEM_ERROR,
    STATUS_OUT_OF_MEM,
    STATUS_SOCKET_ERROR,
    STATUS_DEVICE_OFFLINE,
    STATUS_IO_TIMEOUT,
    STATUS_THREAD_ERROR,
    STATUS_REG_ERROR,
    STATUS_NO_MORE_DATA,
	STATUS_LOAD_RESOURCE_ERROR,
    STATUS_UNKNOWN_ERROR
};

enum eCMD
{
    MIME_OPEN_SESSION,
    MIME_CLOSE_SESSION,
    MIME_NEW_CHAR,
    MIME_DEVICE_CONNECTING,
    MIME_DEVICE_CONNECTED,
    MIME_DEVICE_OFFLINE,
    MIME_DEVICE_NONE,
    MIME_CLIENT_ACTIVE,
    MIME_CLIENT_SELECT,
    MIME_CLIENT_UI_POS,
    MIME_PING,
    MIME_CHECK_PROCESS,
    MIME_EXIT_NOW,
    MIME_SOCKET_CLOSE,
    SOCKET_NEW_ADB_SOCK,
    SOCKET_NEW_WIFI_SOCK,
    SOCKET_REQUEST_ID,
    SOCKET_CLIENT_ID,
	MIME_GET_SERVER_PID,
	MIME_PING_RESPONSE,
	MIME_STATUS_BAR_ERROR
};

enum eConnType
{
    CNN_TYPE_USB,
    CNN_TYPE_WIFI,
	CNN_TYPE_NONE
};

}

#endif