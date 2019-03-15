#ifndef __MSG_QUEUE_H__
#define __MSG_QUEUE_H__

#include "common.h"
#include "packet.h"
#include "SecurityAttribute.h"

namespace DroidPad
{

class CMessageQueue;

struct Message
{
  friend class CMessageQueue;

  struct MsgComp
  {
    bool operator()(const std::shared_ptr<Message> &lh, const std::shared_ptr<Message> &rh)
    {
      return lh->when < rh->when;
    }
  };

  struct Deleter
  {
    void operator()(Message *msg) const
    {
      delete msg;
    }
  };
  static std::shared_ptr<Message> create();

  PACKET cookie;
  u_long when;
  PACKET reply;

private:
  void becomeSync();
  void wait();
  void signal();

  Message() : m_hSyncEvent(NULL) {}
  Message(const Message &) {}
  ~Message();
  HANDLE m_hSyncEvent;
};

//////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////
class CMessageQueue
{
public:
  class MessageHandler
  {
  public:
    virtual void OnHandleMessage(std::shared_ptr<Message> msg) = 0;
  };

public:
  CMessageQueue(MessageHandler *hd);
  ~CMessageQueue();

  void loop();
  void quit();

  void sendMessage(std::shared_ptr<Message> msg, u_long delay = 0);

  void postMessage(PACKET *packet, u_long delay = 0);
  void postMessage(std::shared_ptr<Message> msg, u_long delay = 0);
  void postMessageAtFront(std::shared_ptr<Message> msg, u_long delay = 0);

private:
  void processMessage();
  void unLockAndWaitRelative(u_long time);
  inline void lockSC() { EnterCriticalSection(&cs); }
  inline void unLockSC() { LeaveCriticalSection(&cs); }
  typedef std::deque<std::shared_ptr<Message>> TMsgQueue;
  TMsgQueue msgQueue;
  MessageHandler *m_hMsgHandler;
  HANDLE m_hWakeUpEvent;
  bool m_bPendingExit;
  bool m_bSuspended;
  CRITICAL_SECTION cs;
};

}

#endif