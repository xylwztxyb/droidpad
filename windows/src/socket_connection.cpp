#include "common.h"
#include "socket_connection.h"
#include "packet.h"
#include "util.h"

using namespace DroidPad;

static void _try_get_client_id(SOCKET sock)
{
	PACKET p;
	p.setCMD(SOCKET_REQUEST_ID);
	send(sock, p.getBuff(), p.buffLength(), 0);
}

static void _notify_connected(CMessageQueue *queue, eConnType type)
{
	PACKET p;
	p.setCMD(MIME_DEVICE_CONNECTED);
	p.write(type);
	queue->postMessage(&p);
}

static void _notify_device_offline(CMessageQueue *queue)
{
	PACKET p;
	p.setCMD(MIME_DEVICE_OFFLINE);
	queue->postMessage(&p);
}

///////////////////////////////////////////////////////////////////

namespace DroidPad
{

CSocketConnection::CSocketConnection(SOCKET sock, eConnType type, int client_id,
								   CMessageQueue *main_queue) : m_sock(sock),
															   m_pMainQueue(main_queue),
															   m_nClientTimeoutCount(0),
															   m_eConnType(type),
															   m_iClientId(client_id)

{
}

CSocketConnection::~CSocketConnection()
{
}

bool CSocketConnection::OnCreateThread()
{
	int recv_timeout = SOCKET_RCV_TIMEOUT;
	if (setsockopt(m_sock, SOL_SOCKET, SO_RCVTIMEO, (char *)&recv_timeout,
		sizeof(recv_timeout)) == SOCKET_ERROR)
	{
		PRINT_FUNC_ERROR("SocketConnection::OnCreateThread");
		return false;
	}
	return true;
}

void CSocketConnection::OnStartThread()
{
	_notify_connected(m_pMainQueue, m_eConnType);
}

void CSocketConnection::OnDestroyThread()
{
	closesocket(m_sock);
}

eSTATUS _handle_wsa_error(CSocketConnection *st, int err)
{
	switch (err)
	{
	case WSAETIMEDOUT:
	{
		st->m_nClientTimeoutCount++;
		if (st->m_nClientTimeoutCount > SOCKET_PING_THRSHD)
		{
			st->m_nClientTimeoutCount = 0;
			return st->_ping_socket() ? STATUS_NO_ERROR : STATUS_DEVICE_OFFLINE;
		}
		return STATUS_NO_ERROR;
	}
	case WSAECONNRESET:
	case WSAECONNABORTED:
		return STATUS_DEVICE_OFFLINE;
	default:
		return STATUS_UNKNOWN_ERROR;
	}
}

void CSocketConnection::_handle_packet(PACKET &p)
{
	switch (p.getCMD())
	{
	case SOCKET_CLIENT_ID:
	{
		int id;
		p.read(id);
		m_iClientId = id;
		break;
	}
	case MIME_NEW_CHAR:
		m_pMainQueue->postMessage(&p);
		break;
	case MIME_PING:
		p.setCMD(MIME_PING_RESPONSE);
		send(m_sock, p.getBuff(), p.buffLength(), 0);
		break;
	}
}

bool CSocketConnection::_ping_socket()
{
	PACKET p;
	p.setCMD(MIME_PING);
	//If send() returns error, the client is dead.
	if(send(m_sock, p.getBuff(), p.buffLength(), 0) == SOCKET_ERROR)
		return false;
	//If recv() returns any valid data, the client is alive, else the client is dead.
	int ret_size = recv(m_sock, p.getBuff(), p.buffLength(), 0);
	if(ret_size > 0)
	{
		_handle_packet(p);
		return true;
	}
	return false;
}

bool CSocketConnection::threadLoop()
{
	if(m_iClientId == 0)
		_try_get_client_id(m_sock);

	PACKET p;
	int ret = recv(m_sock, p.getBuff(), p.buffLength(), 0);
	switch (ret)
	{
	case 0:
		_notify_device_offline(m_pMainQueue);
		return false;

	case SOCKET_ERROR:
	{
		int _last_err = WSAGetLastError();
		if (_handle_wsa_error(this, _last_err) == STATUS_DEVICE_OFFLINE)
		{
			_notify_device_offline(m_pMainQueue);
			return false;
		}
		break;
	}

	default:
		_handle_packet(p);
	}
	return true;
}

}