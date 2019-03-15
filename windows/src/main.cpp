#include "common.h"
#include "native_client.h"
#include "util.h"
#include "msg_queue.h"
#include "socket_connection.h"
#include "ime_status_bar.h"
#include "net_monitor.h"
#include "phone_detector.h"

using namespace std;
using namespace DroidPad;

class MainThread;

////////////////////////////////////////////////////////
typedef map<SESSIONID, CNativeClient *> IMEClientMap;
IMEClientMap gIMEClients;
SESSIONID gActiveSession;
CMessageQueue *gQueue = NULL;
CSocketConnection *gConnection = NULL;
CNativeClientPool *gNativeClientPool = NULL;
CNetMonitor *gNetMonitor = NULL;
CPhoneDetector *gPhoneDetector = NULL;
MainThread *gMainThread = NULL;

///////////////////////////////////////////////////////
static SESSIONID OpenSession(HANDLE remote_handle)
{
    SESSIONID id;
    CNativeClient *client = gNativeClientPool->obtain();
    if (client)
    {
        client->setMessageQueue(gQueue);
        client->setRemoteHandle(remote_handle);
    }
    else
        client = CNativeClient::create(genRandomID(), gQueue, remote_handle);
    id = client->getSessionID();
    ASSERT(client->resumeThread());
    gIMEClients[id] = client;
    return id;
}

static void CloseSession(const SESSIONID id)
{
    if (gIMEClients.find(id) != gIMEClients.end())
    {
        gIMEClients.at(id)->terminate();
        gIMEClients.erase(id);
    }
}

static void ClientActive(const SESSIONID id, bool active)
{
    if (gIMEClients.find(id) != gIMEClients.end())
    {
        if (active)
        {
            gActiveSession = id;
            CNativeClient *client = gIMEClients.at(id);
            if (gConnection)
                client->IMEOnDeviceConnected(gConnection->getConnType());
            else
                client->IMEOnDeviceNone();
        }
        else
            gActiveSession.clear();
    }
}

static void ClientSelect(const SESSIONID id, bool select)
{
    if (gIMEClients.find(id) != gIMEClients.end())
    {
        if (select)
        {
            gActiveSession = id;
            CNativeClient *client = gIMEClients.at(id);
            if (gConnection)
                client->IMEOnDeviceConnected(gConnection->getConnType());
            else
                client->IMEOnDeviceNone();
        }
        else
            gActiveSession.clear();
    }
}

static void _checkClient(const SESSIONID id)
{
    if (gIMEClients.find(id) != gIMEClients.end())
    {
        if (!isProcAlive(gIMEClients.at(id)->getRemoteHandle()))
        {
            gIMEClients.at(id)->terminate();
            gIMEClients.erase(id);
        }
    }
}

static void _updateDeviceState(int state, eConnType type = CNN_TYPE_NONE)
{
    if (!gActiveSession.empty() &&
        gIMEClients.find(gActiveSession) != gIMEClients.end())
    {
        CNativeClient *client = gIMEClients.at(gActiveSession);
        switch (state)
        {
        case MIME_DEVICE_CONNECTED:
            client->IMEOnDeviceConnected(type);
            break;
        case MIME_DEVICE_CONNECTING:
            client->IMEOnDeviceConnecting();
            break;
        case MIME_DEVICE_OFFLINE:
            client->IMEOnDeviceConnectionFail();
            break;
        case MIME_DEVICE_NONE:
            client->IMEOnDeviceNone();
            break;
        }
    }
}

class MainMessageHandler : public CMessageQueue::MessageHandler
{
  public:
    void OnHandleMessage(shared_ptr<Message> msg) override
    {

        switch (msg->cookie.getCMD())
        {
        case MIME_OPEN_SESSION:
        {
            HANDLE remote_handle;
            msg->cookie.read(remote_handle);
            const SESSIONID id = OpenSession(remote_handle);
            msg->reply.write(id);
            break;
        }

        case MIME_CLOSE_SESSION:
        {
            SESSIONID id;
            msg->cookie.read(id);
            CloseSession(id);
            break;
        }

        case MIME_CLIENT_ACTIVE:
        {
            SESSIONID id;
            msg->cookie.read(id);
            bool active;
            msg->cookie.read(active);
            ClientActive(id, active);
            break;
        }

        case MIME_CLIENT_SELECT:
        {
            SESSIONID id;
            msg->cookie.read(id);
            bool select;
            msg->cookie.read(select);
            ClientSelect(id, select);
            break;
        }

        case MIME_DEVICE_CONNECTED:
        {
            eConnType type;
            msg->cookie.read(type);
            _updateDeviceState(MIME_DEVICE_CONNECTED, type);
            break;
        }

        case MIME_DEVICE_OFFLINE:
        {
            _updateDeviceState(MIME_DEVICE_OFFLINE);
            if (gConnection)
            {
                gConnection->terminate();
                gConnection = NULL;
            }
            gNetMonitor->resumeThread();
            break;
        }

        case MIME_DEVICE_CONNECTING:
            _updateDeviceState(MIME_DEVICE_CONNECTING);
            break;

        case MIME_DEVICE_NONE:
        {
            if (gConnection)
                _updateDeviceState(MIME_DEVICE_CONNECTED, gConnection->getConnType());
            else
                _updateDeviceState(MIME_DEVICE_NONE);
            break;
        }

        case MIME_NEW_CHAR:
        {
            if (!gActiveSession.empty() &&
                gIMEClients.find(gActiveSession) != gIMEClients.end())
            {
                std::wstring wstr;
                msg->cookie.read(wstr);
                gIMEClients.at(gActiveSession)->IMEOnNewWords(wstr);
            }
            break;
        }
        case MIME_CHECK_PROCESS:
        {
            SESSIONID id;
            msg->cookie.read(id);
            _checkClient(id);
            break;
        }
        case SOCKET_NEW_ADB_SOCK:
        {
            SOCKET socket;
            msg->cookie.read(socket);
            if (!gConnection)
            {
                gConnection = new CSocketConnection(socket, CNN_TYPE_USB, 0, gQueue);
                if (!gConnection->initialize() || !gConnection->resumeThread())
                {
                    closesocket(socket);
                    delete gConnection;
                    gConnection = NULL;
                }
            }
            else
            {
                closesocket(socket);
                _updateDeviceState(MIME_DEVICE_CONNECTED, gConnection->getConnType());
            }

            break;
        }
        case SOCKET_NEW_WIFI_SOCK:
        {
            SOCKET socket;
            int client_id;
            msg->cookie.read(socket);
            msg->cookie.read(client_id);
            if (!gConnection)
            {
                gConnection = new CSocketConnection(socket, CNN_TYPE_WIFI, client_id, gQueue);
                if (!gConnection->initialize() || !gConnection->resumeThread())
                {
                    closesocket(socket);
                    delete gConnection;
                    gConnection = NULL;
                    _updateDeviceState(MIME_DEVICE_NONE);
                    gNetMonitor->resumeThread();
                }
            }
            else
            {
                if (gConnection->getConnType() == CNN_TYPE_USB)
                {
                    if (gConnection->getClientId() == client_id)
                    {
                        CSocketConnection *tmp = new CSocketConnection(socket, CNN_TYPE_WIFI, client_id, gQueue);
                        if (tmp->initialize() && tmp->resumeThread())
                        {
                            gConnection->terminate();
                            delete gConnection;
                            gConnection = tmp;
                        }
                        else
                        {
                            closesocket(socket);
                            delete tmp;
                            _updateDeviceState(MIME_DEVICE_CONNECTED, gConnection->getConnType());
                            gNetMonitor->resumeThread();
                        }
                    }
                    else
                    {
                        closesocket(socket);
                        _updateDeviceState(MIME_DEVICE_CONNECTED, gConnection->getConnType());
                        gNetMonitor->resumeThread();
                    }
                }
                else
                {
                    closesocket(socket);
                    _updateDeviceState(MIME_DEVICE_CONNECTED, gConnection->getConnType());
                }
            }
            break;
        }
        case MIME_SOCKET_CLOSE:
            _updateDeviceState(MIME_DEVICE_NONE);
            break;
        case MIME_STATUS_BAR_ERROR:
        {
            std::wstring str;
            msg->cookie.read(str);
            gPhoneDetector->showMessage(str);
        }
        break;
        }
    }
};

/////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////

class MainThread : public CIPCThread
{
  public:
    MainThread(SESSIONID id, CMessageQueue *main_queue);
    void OnDataReceived(PACKET &data, PACKET *reply) override;

  private:
    CMessageQueue *mainQueue;
};

MainThread::MainThread(SESSIONID id, CMessageQueue *main_queue) : CIPCThread(id.c_str(), false),
                                                                  mainQueue(main_queue)
{
}

void MainThread::OnDataReceived(PACKET &data, PACKET *reply)
{
    switch (data.getCMD())
    {
    case MIME_OPEN_SESSION:
    {
        HANDLE remote_handle;
        data.read(remote_handle);
        shared_ptr<Message> msg = Message::create();
        msg->cookie.setCMD(MIME_OPEN_SESSION);
        msg->cookie.write(remote_handle);
        mainQueue->sendMessage(msg);
        SESSIONID id;
        msg->reply.read(id);
        reply->write(id);
        break;
    }
    case MIME_CLOSE_SESSION:
    {
        int start = GetTickCount();
        SESSIONID id;
        data.read(id);
        shared_ptr<Message> msg = Message::create();
        msg->cookie.setCMD(MIME_CLOSE_SESSION);
        msg->cookie.write(id);
        mainQueue->sendMessage(msg);
        break;
    }
    case MIME_GET_SERVER_PID:
    {
        reply->write(GetCurrentProcessId());
        break;
    }
    default:
        CIPCThread::OnDataReceived(data, reply);
    }
}

/////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////

void cleanUp()
{
    Log::D("Program exiting, please wait for seconds\n");
    if (gConnection)
    {
        if (gConnection->isAlive())
            gConnection->terminate();
        delete gConnection;
        gConnection = NULL;
    }
    if (gNetMonitor)
    {
        if (gNetMonitor->isAlive())
            gNetMonitor->terminate();
        delete gNetMonitor;
        gNetMonitor = NULL;
    }

    if (gPhoneDetector)
    {
        if (gPhoneDetector->isAlive())
            gPhoneDetector->terminate();
        delete gPhoneDetector;
        gPhoneDetector = NULL;
    }

    if (gMainThread)
    {
        if (gMainThread->isAlive())
            gMainThread->terminate();
        delete gMainThread;
        gMainThread = NULL;
    }

    for (IMEClientMap::iterator it = gIMEClients.begin(); it != gIMEClients.end(); it++)
        it->second->terminate();

    if (gNativeClientPool)
        delete gNativeClientPool;
    gNativeClientPool = NULL;

    if (gQueue)
    {
        gQueue->quit();
        delete gQueue;
    }
    gQueue = NULL;

    unMarkServerStarted();
}

BOOL WINAPI CtrlHandler(DWORD fdwCtrlType)
{
    switch (fdwCtrlType)
    {
        // Handle the CTRL-C signal.
    case CTRL_C_EVENT:
        cleanUp();
        Beep(750, 300);
        return TRUE;

        // CTRL-CLOSE: confirm that the user wants to exit.
    case CTRL_CLOSE_EVENT:
        cleanUp();
        Beep(600, 200);
        return TRUE;

        // Pass other signals to the next handler.
    case CTRL_BREAK_EVENT:
        Beep(900, 200);
        return FALSE;

    case CTRL_LOGOFF_EVENT:
        cleanUp();
        Beep(1000, 200);
        return FALSE;

    case CTRL_SHUTDOWN_EVENT:
        Beep(750, 500);
        printf("Ctrl-Shutdown event\n\n");
        return TRUE;

    default:
        return FALSE;
    }
}

int main()
{
    if (isServerStarted())
        return -1;
    if (!SetConsoleCtrlHandler(CtrlHandler, TRUE))
        return -1;
    do
    {
        if (markServerStarted() != STATUS_NO_ERROR)
            break;
        //First of all, initialize the NativeClientPool
        gNativeClientPool = new CNativeClientPool(NATIVE_CLIENT_POOL_MAX);
        //start main thread to communicate with ime client
        MainMessageHandler handler;
        gQueue = new CMessageQueue(&handler);
        gMainThread = new MainThread(WRITING_PAD_SERVICE_NAME, gQueue);
        if (!gMainThread->initialize() || !gMainThread->resumeThread())
            break;
        //start net monitor thread to communicate with device.
        gNetMonitor = new CNetMonitor(gQueue);
        if (!gNetMonitor->initialize() || !gNetMonitor->resumeThread())
            break;
        //start phone detector
        gPhoneDetector = new CPhoneDetector();
        if (!gPhoneDetector->initialize() || !gPhoneDetector->resumeThread())
            break;
        //start loop the message queue.
        gQueue->loop();

    } while (0);

    cleanUp();
    return 0;
}
