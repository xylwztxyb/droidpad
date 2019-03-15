#ifndef native_client_pool_h__
#define native_client_pool_h__

#include "native_client.h"
#include <vector>

namespace DroidPad
{

class CNativeClientPool
{
	friend class CNativeClient;

  public:
	CNativeClientPool(int max);
	~CNativeClientPool();
	CNativeClient *obtain();

  private:
	void recycle(CNativeClient *);
	int m_iPoolMax;
	int m_iClientCount;
	std::vector<CNativeClient *> m_IdleClients;
};

}

#endif // native_client_pool_h__
