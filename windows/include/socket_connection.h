#ifndef __socket_connection_h__
#define __socket_connection_h__

#include "common.h"
#include "msg_queue.h"
#include "thread_base.h"

namespace DroidPad
{

#define SOCKET_RCV_TIMEOUT 1000
#define SOCKET_SND_TIMEOUT 1000
#define SOCKET_PING_THRSHD 5

class CSocketConnection : public CThreadBase
{
  public:
	CSocketConnection(SOCKET sock, eConnType type, int client_id, CMessageQueue *main_queue);
	~CSocketConnection();
	eConnType getConnType()  { return m_eConnType; };
	int getClientId()  { return m_iClientId; };

  private:
	virtual bool threadLoop() override;
	virtual void OnDestroyThread() override;
	virtual bool OnCreateThread() override;
	virtual void OnStartThread() override;
	bool _ping_socket();
	void _handle_packet(PACKET &p);
	SOCKET m_sock;
	eConnType m_eConnType;
	int m_iClientId;
	CMessageQueue *m_pMainQueue;
	short m_nClientTimeoutCount;

	friend eSTATUS _handle_wsa_error(CSocketConnection *sc, int err);
};

}

#endif