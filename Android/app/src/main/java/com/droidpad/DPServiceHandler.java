package com.droidpad;

import android.os.Handler;
import android.os.Message;

import java.io.IOException;
import java.net.Socket;
import java.util.ArrayList;
import java.util.List;

class DPServiceHandler extends Handler {

    private static final String TAG = "DPServiceHandler";
    private static DPServiceHandler sInstance = null;

    private SocketConnection mConnection;
    private List<IDPServiceStateCallback> mCallbacks;

    synchronized static DPServiceHandler create() {
        if (sInstance == null)
            sInstance = new DPServiceHandler();
        return sInstance;
    }

    void destroy() {
        if (mConnection != null)
            mConnection.quit();
        removeCallbacksAndMessages(null);
        sInstance = null;
    }

    private DPServiceHandler() {
        mCallbacks = new ArrayList<>();
    }

    void addDPServiceStateCallback(IDPServiceStateCallback callback) {
        mCallbacks.add(callback);
    }

    void removeDPServiceStateCallback(IDPServiceStateCallback callback) {
        mCallbacks.remove(callback);
    }

    private void closeSocket(Socket sock) {
        try
        {
            sock.close();
        } catch (IOException e)
        {
            e.printStackTrace();
        }
    }

    private void notifySocketConnected(int type) {
        if (!mCallbacks.isEmpty())
        {
            for (IDPServiceStateCallback callback : mCallbacks)
                callback.onSocketConnected(type);
        }
    }

    private void notifySocketConnNone() {
        if (!mCallbacks.isEmpty())
        {
            for (IDPServiceStateCallback callback : mCallbacks)
                callback.onSocketConnNone();
        }
    }

    private void notifyCurrentState(IDPServiceStateCallback caller) {
        if (caller == null)
            return;
        if (mConnection != null && mConnection.isAlive())
            caller.onSocketConnected(mConnection.getType());
        else
            caller.onSocketConnNone();
    }

    private void notifyPingFromServer(int type) {
        if (!mCallbacks.isEmpty())
        {
            for (IDPServiceStateCallback callback : mCallbacks)
            {
                callback.onPingFromServer(type);
            }
        }
    }

    private void notifySocketThreadInitError(int type) {
        if (!mCallbacks.isEmpty())
        {
            for (IDPServiceStateCallback callback : mCallbacks)
            {
                callback.onSocketThreadInitError(type);
            }
        }
    }

    private HandleCnnResult handleNewConnection(Socket sock, int type) {

        switch (type)
        {
            case Constants.SocketType.TYPE_WIFI:
            {
                if (mConnection == null)
                {
                    mConnection = SocketConnection.create(sock, this, type);
                    return mConnection == null ? HandleCnnResult.CONNECTION_ERROR : HandleCnnResult.SUCCESS;
                } else
                {
                    if (mConnection.getType() == Constants.SocketType.TYPE_ADB)
                    {
                        SocketConnection conn = SocketConnection.create(sock, this, type);
                        if (conn != null)
                        {
                            mConnection.kickOut();
                            mConnection = conn;
                            return HandleCnnResult.SUCCESS;
                        } else
                            return HandleCnnResult.CONNECTION_ERROR;
                    } else
                        return HandleCnnResult.CONNECTION_ALREADY_EXISTS;
                }
            }
            case Constants.SocketType.TYPE_ADB:
            {
                if (mConnection == null)
                {
                    mConnection = SocketConnection.create(sock, this, type);
                    return mConnection == null ? HandleCnnResult.CONNECTION_ERROR : HandleCnnResult.SUCCESS;
                } else
                    return HandleCnnResult.CONNECTION_ALREADY_EXISTS;
            }
            default:
                return HandleCnnResult.CONNECTION_ERROR;
        }
    }

    @Override
    public void handleMessage(Message msg) {
        switch (msg.what)
        {
            case Constants.Command.CMD_NEW_COMPOSED_WORDS:
            {
                Packet p = new Packet();
                p.setCmd(Constants.Command.MIME_CMD_NEW_WORDS);
                p.writeStringUnicode((String) msg.obj);
                if (mConnection != null)
                    mConnection.send(p);
                break;
            }
            case Constants.Command.CMD_CONNECTION_BROKEN:
            {
                if (mConnection != null)
                {
                    mConnection.quit();
                    mConnection = null;
                }
                notifySocketConnNone();
                break;
            }
            case Constants.Command.CMD_NEW_CONNECTION:
            {
                Socket sock = (Socket) msg.obj;
                int type = msg.arg1;
                HandleCnnResult res = handleNewConnection(sock, type);
                if (res == HandleCnnResult.CONNECTION_ERROR)
                {
                    closeSocket(sock);
                    notifySocketConnNone();
                } else if (res == HandleCnnResult.CONNECTION_ALREADY_EXISTS)
                {
                    notifySocketConnected(mConnection.getType());
                    closeSocket(sock);
                } else
                    notifySocketConnected(type);
                break;
            }
            case Constants.Command.CMD_REQUEST_CURRENT_STATE:
                notifyCurrentState((IDPServiceStateCallback) msg.obj);
                break;
            case Constants.Command.MIME_CMD_PING:
                notifyPingFromServer(msg.arg1);
                break;
            case Constants.ThreadStatus.STATUS_THREAD_INIT_ERROR:
                notifySocketThreadInitError(msg.arg1);
                break;
            case Constants.Command.CMD_USB_CABLE_PLUGOUT:
                if (mConnection != null && mConnection.getType() == Constants.SocketType.TYPE_ADB)
                {
                    mConnection.quit();
                    mConnection = null;
                    notifySocketConnNone();
                }
                break;
            case Constants.Command.CMD_WIFI_POWER_OFF:
                if (mConnection != null && mConnection.getType() == Constants.SocketType.TYPE_WIFI)
                {
                    mConnection.quit();
                    mConnection = null;
                    notifySocketConnNone();
                }
                break;
            case Constants.Command.CMD_APP_EXIT:
                if (mConnection != null)
                {
                    mConnection.quit();
                    mConnection = null;
                }
                break;
        }
    }

    private enum HandleCnnResult {
        SUCCESS, CONNECTION_ERROR, CONNECTION_ALREADY_EXISTS
    }
}
