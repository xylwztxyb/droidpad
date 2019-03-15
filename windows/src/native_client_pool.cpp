#include "native_client_pool.h"
#include "util.h"

namespace DroidPad
{

#define POOL_STEP 5

CNativeClientPool::CNativeClientPool(int max) : m_iPoolMax(max),
											  m_iClientCount(0)
{
}

CNativeClientPool::~CNativeClientPool()
{
	std::vector<CNativeClient *>::iterator it;
	for (it = m_IdleClients.begin(); it != m_IdleClients.end(); it++)
	{
		if ((*it)->isAlive())
			(*it)->resumeThread();
		(*it)->destroy();
	}
}

CNativeClient *CNativeClientPool::obtain()
{
	CNativeClient *client = NULL;
	if (m_IdleClients.size() > 0)
	{
		std::vector<CNativeClient *>::iterator it = m_IdleClients.begin();
		client = *it;
		m_IdleClients.erase(it);
		client->resumeThread();
	}
	else if (m_iClientCount < m_iPoolMax)
	{
		int n = m_iPoolMax - m_iClientCount > POOL_STEP ? POOL_STEP : m_iPoolMax - m_iClientCount;
		for (int i = 0; i < n - 1; i++)
		{
			CNativeClient *c = new CNativeClient(genRandomID(), this);
			ASSERT(c->initialize());
			m_IdleClients.push_back(c);
		}
		client = new CNativeClient(genRandomID(), this);
		ASSERT(client->initialize());
		ASSERT(client->resumeThread());
		m_iClientCount += n;
	}
	return client;
}

void CNativeClientPool::recycle(CNativeClient *client)
{
	client->suspendThread();
	m_IdleClients.push_back(client);
}

}