package com.droidpad;

import android.os.Handler;
import android.os.Message;
import android.util.Log;

import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.net.Socket;
import java.net.SocketTimeoutException;

public class SocketConnection extends SocketThreadBase {

    private static final String TAG = "SocketConnection";

    private Socket mSocket;
    private InputStream mIn;
    private OutputStream mOut;
    private Handler mHandler;
    private int mConnType;
    private volatile boolean bKickedOut = false;

    private int mTimeoutCount = 0;

    private SocketConnection(Socket sock, Handler mainHandler, int connType) {
        super("SocketConnection");
        mSocket = sock;
        mHandler = mainHandler;
        mConnType = connType;
    }

    static SocketConnection create(Socket sock, Handler handler, int connType) {
        SocketConnection conn = new SocketConnection(sock, handler, connType);
        if (conn.initSocket())
        {
            conn.start();
            return conn;
        }
        return null;
    }

    @Override
    protected boolean initSocket() {
        try
        {
            mSocket.setSoTimeout(1000);
            mIn = mSocket.getInputStream();
            mOut = mSocket.getOutputStream();
        } catch (IOException e)
        {
            e.printStackTrace();
            return false;
        }
        return true;
    }

    @Override
    protected boolean threadLoop() {

        if (mTimeoutCount > Constants.SocketArg.SOCKET_TIMEOUT_THRESHOLD)
        {
            mTimeoutCount = 0;
            Packet p = Util.pingServer(mSocket);
            if (p == null)
            {
                notifyConnBroken();
                return false;
            } else
            {
                handlePacket(p);
                return true;
            }
        }

        Packet p = new Packet();
        int p_size = p._buffer.get().length;
        try
        {
            int _bytes = mIn.read(p._buffer.get());
            if (_bytes < p_size)
            {
                mTimeoutCount++;
                return true;
            }

        } catch (SocketTimeoutException e)
        {
            mTimeoutCount++;
            return true;
        } catch (IOException e)
        {
            notifyConnBroken();
            return false;
        }

        handlePacket(p);
        return true;
    }

    @Override
    protected void onThreadQuit() {
        if (!mSocket.isClosed())
        {
            try
            {
                mSocket.close();
            } catch (IOException e)
            {
                e.printStackTrace();
            }
        }
    }

    private void handlePacket(Packet packet) {
        switch (packet.getCmd())
        {
            case Constants.Command.MIME_CMD_PING:
            {
                //Just send the packet back to server
                packet.setCmd(Constants.Command.MIME_CMD_PING_RESPONSE);
                send(packet);
                Message msg = Message.obtain();
                msg.what = Constants.Command.MIME_CMD_PING;
                msg.arg1 = mConnType;
                mHandler.sendMessage(msg);
                break;
            }
            case Constants.Command.MIME_CMD_REQUEST_ID_FROM_SERVER:
            {
                Packet p = new Packet();
                p.setCmd(Constants.Command.MIME_CMD_ID_OF_CLIENT);
                p.writeInt(DroidPadApp.ID);
                send(p);
                break;
            }
        }
    }

    void send(Packet p) {
        try
        {
            mOut.write(p._buffer.get());
        } catch (IOException e)
        {
            e.printStackTrace();
            notifyConnBroken();
        }
    }

    void kickOut() {
        bKickedOut = true;
    }

    private void notifyConnBroken() {
        if (!bKickedOut)
            mHandler.sendEmptyMessage(Constants.Command.CMD_CONNECTION_BROKEN);
    }

    public int getType() {
        return mConnType;
    }
}
