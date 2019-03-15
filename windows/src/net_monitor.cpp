#include "net_monitor.h"
#include "util.h"

using namespace DroidPad;

static bool _send_udp_pack(SOCKET sock, sockaddr *addr, int addr_len, BroadcastPack pack)
{
	int res = sendto(sock, (char *)&pack, sizeof(pack), 0, addr, addr_len);
	return res < 1 ? false : true;
}

static int _setSocketNoneBlock(SOCKET sock, bool none_block)
{
	u_long ul = none_block;
	return ioctlsocket(sock, FIONBIO, &ul);
}

static SOCKET _accept_connection(SOCKET sock)
{
	if (_setSocketNoneBlock(sock, true) == SOCKET_ERROR)
	{
		PRINT_FUNC_ERROR_WSA("NetMonitor::_accept_connection");
		return INVALID_SOCKET;
	}

	SOCKET new_client = accept(sock, NULL, NULL);
	if (new_client != INVALID_SOCKET)
	{
		_setSocketNoneBlock(sock, false);
		//A socket accepted from none-block socket, will be none-block too.
		//so we set the new socket blocked.
		_setSocketNoneBlock(new_client, false);
		return new_client;
	}

	int err = WSAGetLastError();
	if (err != WSAEWOULDBLOCK)
	{
		_setSocketNoneBlock(sock, false);
		return INVALID_SOCKET;
	}

	fd_set read_set;
	FD_ZERO(&read_set);
	FD_SET(sock, &read_set);

	timeval timout = {2, 0};
	int ret = select(0, &read_set, NULL, NULL, &timout);
	if (ret <= 0)
	{
		_setSocketNoneBlock(sock, false);
		return INVALID_SOCKET;
	}

	if ((new_client = accept(sock, NULL, NULL)) == INVALID_SOCKET)
	{
		_setSocketNoneBlock(sock, false);
		return INVALID_SOCKET;
	}

	_setSocketNoneBlock(sock, false);
	_setSocketNoneBlock(new_client, false);
	return new_client;
}

static void _notify_connecting(CMessageQueue *queue)
{
	PACKET p;
	p.setCMD(MIME_DEVICE_CONNECTING);
	queue->postMessage(&p);
}

static void _notify_none_device(CMessageQueue *queue)
{
	PACKET p;
	p.setCMD(MIME_DEVICE_NONE);
	queue->postMessage(&p);
}

//////////////////////////////////////////////////////////////////

namespace DroidPad
{

CNetMonitor::CNetMonitor(CMessageQueue *queue) : m_pMainQueue(queue),
											  adbSock(INVALID_SOCKET),
											  BrdcstSock(INVALID_SOCKET),
											  wifiSock(INVALID_SOCKET)
{
}

CNetMonitor::~CNetMonitor()
{
}

bool CNetMonitor::OnCreateThread()
{
	WSADATA wsaData;
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != NO_ERROR)
	{
		PRINT_FUNC_ERROR_WSA("NetMonitor::OnCreateThread");
		return false;
	}
	do
	{
		//setup adb socket
		adbSock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		if (adbSock == INVALID_SOCKET)
		{
			PRINT_FUNC_ERROR_WSA("NetMonitor::OnCreateThread");
			break;
		}
		char so_reuse = 1;
		if (setsockopt(adbSock, SOL_SOCKET, SO_REUSEADDR, &so_reuse, sizeof(so_reuse)) == SOCKET_ERROR)
		{
			PRINT_FUNC_ERROR_WSA("NetMonitor::OnCreateThread");
			break;
		}
		sockaddr_in adb_sin;
		memset(&adb_sin, 0, sizeof(adb_sin));
		adb_sin.sin_family = AF_INET;
		adb_sin.sin_port = htons(ADB_LISTEN_PORT);
		adb_sin.sin_addr.S_un.S_addr = htonl(INADDR_ANY);

		if (bind(adbSock, (sockaddr *)&adb_sin, sizeof(adb_sin)) == SOCKET_ERROR)
		{
			PRINT_FUNC_ERROR_WSA("NetMonitor::OnCreateThread");
			break;
		}
		if (listen(adbSock, 10) == SOCKET_ERROR)
		{
			PRINT_FUNC_ERROR_WSA("NetMonitor::OnCreateThread");
			break;
		}

		//setup wifi broadcast socket
		BrdcstSock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
		if (BrdcstSock == INVALID_SOCKET)
		{
			PRINT_FUNC_ERROR_WSA("NetMonitor::OnCreateThread");
			break;
		}
		if (setsockopt(BrdcstSock, SOL_SOCKET, SO_REUSEADDR, &so_reuse, sizeof(so_reuse)) == SOCKET_ERROR)
		{
			PRINT_FUNC_ERROR_WSA("NetMonitor::OnCreateThread");
			break;
		}

		sockaddr_in brdcst_sin;
		memset(&brdcst_sin, 0, sizeof(brdcst_sin));
		brdcst_sin.sin_family = AF_INET;
		brdcst_sin.sin_port = htons(WIFI_LISTEN_PORT);
		brdcst_sin.sin_addr.S_un.S_addr = htonl(INADDR_ANY);

		if (bind(BrdcstSock, (sockaddr *)&brdcst_sin, sizeof(brdcst_sin)) == SOCKET_ERROR)
		{
			PRINT_FUNC_ERROR_WSA("NetMonitor::OnCreateThread");
			break;
		}

		//setup wifi socket to wait connection
		wifiSock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		if (wifiSock == INVALID_SOCKET)
		{
			PRINT_FUNC_ERROR_WSA("NetMonitor::OnCreateThread");
			break;
		}

		sockaddr_in wifi_sin;
		memset(&wifi_sin, 0, sizeof(wifi_sin));
		wifi_sin.sin_family = AF_INET;
		wifi_sin.sin_port = 0;
		wifi_sin.sin_addr.S_un.S_addr = htonl(INADDR_ANY);

		if (bind(wifiSock, (sockaddr *)&wifi_sin, sizeof(wifi_sin)) == SOCKET_ERROR)
		{
			PRINT_FUNC_ERROR_WSA("NetMonitor::OnCreateThread");
			break;
		}
		if (listen(wifiSock, 10) == SOCKET_ERROR)
		{
			PRINT_FUNC_ERROR_WSA("NetMonitor::OnCreateThread");
			break;
		}
		return true;
	} while (0);

	if (adbSock != INVALID_SOCKET)
		closesocket(adbSock);
	if (BrdcstSock != INVALID_SOCKET)
		closesocket(BrdcstSock);
	if (wifiSock != INVALID_SOCKET)
		closesocket(wifiSock);
	adbSock = INVALID_SOCKET;
	BrdcstSock = INVALID_SOCKET;
	wifiSock = INVALID_SOCKET;
	WSACleanup();
	return false;
}

bool CNetMonitor::threadLoop()
{
	static fd_set fds_read;
	FD_ZERO(&fds_read);
	FD_SET(adbSock, &fds_read);
	FD_SET(BrdcstSock, &fds_read);
	timeval tv = {2, 0};
	int ret = select(0, &fds_read, NULL, NULL, &tv);
	if (ret > 0)
	{
		if (FD_ISSET(adbSock, &fds_read))
		{
			_notify_connecting(m_pMainQueue);
			SOCKET client = accept(adbSock, NULL, NULL);
			if (client != INVALID_SOCKET)
			{
				PACKET p;
				p.setCMD(SOCKET_NEW_ADB_SOCK);
				p.write(client);
				m_pMainQueue->postMessage(&p);
				Log::D("NetMonitor:Got new ADB client\n");
			}
		}
		if (FD_ISSET(BrdcstSock, &fds_read))
		{
			do
			{
				sockaddr_in client_sin;
				int sin_len = sizeof(client_sin);
				int ret_size;
				BroadcastPack _pack;
				int _pack_len = sizeof(_pack);
				memset(&client_sin, 0, sizeof(client_sin));

				ret_size = recvfrom(BrdcstSock, (char *)&_pack, _pack_len, 0, (sockaddr *)&client_sin, &sin_len);
				if (ret_size < _pack_len || _pack.cmd != ARE_YOU_MIME_SERVER)
					break;

				int client_id = _pack.data;

				sockaddr_in addr;
				int addr_len = sizeof(addr);
				getsockname(wifiSock, (sockaddr *)&addr, &addr_len);

				_pack.cmd = I_AM_MIME_SERVER;
				_pack.data = ntohs(addr.sin_port);
				if (!_send_udp_pack(BrdcstSock, (sockaddr *)&client_sin, sin_len, _pack))
					break;

				_notify_connecting(m_pMainQueue);
				SOCKET new_sock = _accept_connection(wifiSock);
				if (new_sock == INVALID_SOCKET)
					break;
				PACKET pkt;
				pkt.setCMD(SOCKET_NEW_WIFI_SOCK);
				pkt.write(new_sock);
				pkt.write(client_id);
				m_pMainQueue->postMessage(&pkt);
				Log::D("NetMonitor:Got new wifi client\n");
				suspendInner(); //Suspend right now!
				return true;
			} while (0);
		}
	}
	else
		_notify_none_device(m_pMainQueue);
	return true;
}

void CNetMonitor::OnDestroyThread()
{
	if (adbSock != INVALID_SOCKET)
		closesocket(adbSock);
	if (BrdcstSock != INVALID_SOCKET)
		closesocket(BrdcstSock);
	if (wifiSock != INVALID_SOCKET)
		closesocket(wifiSock);
	adbSock = INVALID_SOCKET;
	BrdcstSock = INVALID_SOCKET;
	wifiSock = INVALID_SOCKET;
	WSACleanup();
}

}
