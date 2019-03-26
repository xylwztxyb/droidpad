package com.droidpad;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.net.ConnectivityManager;
import android.net.NetworkInfo;
import android.os.Handler;
import android.os.HandlerThread;
import android.os.Message;

import java.io.IOException;
import java.net.DatagramPacket;
import java.net.DatagramSocket;
import java.net.InetAddress;
import java.net.InetSocketAddress;
import java.net.Socket;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;

public class WifiSocketProducer extends HandlerThread implements IDPServiceStateCallback {

    private static final String TAG = WifiSocketProducer.class.getSimpleName();

    private Context mContext;
    private DPServiceHandler mServiceHandler;
    private Handler mHandler;
    private volatile boolean bAppHasWifiConnection = false;
    private volatile boolean bQuit = false;

    private DatagramSocket mSocket;
    private ByteBuffer mBuff;
    private DatagramPacket mDatagramPacket;

    private ConnectivityManager mConnManager;
    private WifiBroadcastReceiver mWifiReceiver;
    private Runnable mScanTask = new Runnable() {
        @Override
        public void run() {
            scan();
        }
    };

    WifiSocketProducer(Context context, DPServiceHandler handler) {
        super(WifiSocketProducer.class.getSimpleName());
        mContext = context;
        mServiceHandler = handler;
        mServiceHandler.addDPServiceStateCallback(this);

        mBuff = ByteBuffer.allocate(8);
        mBuff.order(ByteOrder.LITTLE_ENDIAN);
        mDatagramPacket = new DatagramPacket(mBuff.array(), mBuff.capacity());

        mConnManager = (ConnectivityManager) mContext.getSystemService(Context.CONNECTIVITY_SERVICE);
        mWifiReceiver = new WifiBroadcastReceiver();
        mContext.registerReceiver(mWifiReceiver, new IntentFilter(ConnectivityManager.CONNECTIVITY_ACTION));
    }

    boolean initSocket() {
        try
        {
            mSocket = new DatagramSocket();
            mSocket.setSoTimeout(Constants.SocketArg.WIFI_BRDCST_RECV_TIMEOUT);
        } catch (IOException e)
        {
            e.printStackTrace();
            return false;
        }
        return true;
    }

    private void scan() {
        mBuff.clear();
        mBuff.putInt(Constants.BroadcastCommand.ARE_YOU_MIME_SERVER);
        mBuff.putInt(DroidPadApp.ID);
        try
        {
            mDatagramPacket.setPort(Constants.SocketArg.WIFI_BRDCST_PORT);
            mDatagramPacket.setAddress(InetAddress.getByName("255.255.255.255"));
            mSocket.send(mDatagramPacket);
            mSocket.receive(mDatagramPacket);
        } catch (IOException e)
        {
            //e.printStackTrace();
            schedualScan();
            return;
        }
        if (handleBuff())
            mHandler.removeCallbacksAndMessages(null);
        else
        {
            schedualScan();
        }
    }

    private boolean handleBuff() {
        mBuff.rewind();
        int cmd = mBuff.getInt();
        if (cmd == Constants.BroadcastCommand.I_AM_MIME_SERVER)
        {
            int port = mBuff.getInt();
            Socket newSocket = createSocket(mDatagramPacket.getAddress(), port);
            if (newSocket != null)
            {
                handleNewSocket(newSocket);
                return true;
            } else
                return false;
        }
        return false;
    }

    private Socket createSocket(InetAddress addr, int port) {
        Socket sock = new Socket();
        try
        {
            sock.connect(new InetSocketAddress(addr, port),
                    Constants.SocketArg.WIFI_SOCKET_CONNECT_TIMEOUT);
        } catch (IOException e)
        {
            //e.printStackTrace();
            closeSocket(sock);
            return null;
        }
        return sock;
    }

    private void handleNewSocket(Socket socket) {
        Message msg = Message.obtain();
        msg.what = Constants.Command.CMD_NEW_CONNECTION;
        msg.obj = socket;
        msg.arg1 = Constants.SocketType.TYPE_WIFI;
        mServiceHandler.sendMessageAtFrontOfQueue(msg);
    }

    private synchronized void schedualScan() {
        if (!bQuit && mHandler != null && isWifiConnected() && !bAppHasWifiConnection)
        {
            mHandler.removeCallbacksAndMessages(null);
            mHandler.postDelayed(mScanTask, Constants.SocketArg.SOCKET_SCAN_INTERVAL);
        }
    }

    private boolean isWifiConnected() {
        NetworkInfo info = mConnManager.getActiveNetworkInfo();
        return (info != null &&
                info.getType() == ConnectivityManager.TYPE_WIFI &&
                info.isConnected());
    }

    private String getIpAddress(InetAddress addr) {
        StringBuilder builder = new StringBuilder();
        byte[] bs = addr.getAddress();
        for (byte b : bs)
        {
            int ipseg = b < 0 ? 256 + b : b;
            builder.append(ipseg).append(".");
        }
        return builder.toString();
    }

    private void closeSocket(Socket socket) {
        try
        {
            socket.close();
        } catch (IOException e)
        {
            e.printStackTrace();
        }
    }

    @Override
    protected void onLooperPrepared() {
        mHandler = new Handler();
        schedualScan();
    }

    @Override
    public synchronized boolean quit() {
        bQuit = true;
        mContext.unregisterReceiver(mWifiReceiver);
        mServiceHandler.removeDPServiceStateCallback(this);
        mHandler.removeCallbacksAndMessages(null);
        if (!mSocket.isClosed())
            mSocket.close();
        return super.quit();
    }

    @Override
    public void onPingFromServer(int type) {
    }

    @Override
    public void onSocketConnNone() {
        bAppHasWifiConnection = false;
        schedualScan();
    }

    @Override
    public void onSocketConnected(int type) {
        if (type == Constants.SocketType.TYPE_ADB)
            schedualScan();
        else
        {
            bAppHasWifiConnection = true;
            mHandler.removeCallbacksAndMessages(null);
        }
    }

    @Override
    public void onSocketThreadInitError(int type) {
    }

    private void onWifiConnectionChanged(boolean connected) {
        if (!connected)
        {
            bAppHasWifiConnection = false;
            mHandler.removeCallbacksAndMessages(null);
            mServiceHandler.sendEmptyMessage(Constants.Command.CMD_WIFI_POWER_OFF);
        } else
            schedualScan();
    }

    private class WifiBroadcastReceiver extends BroadcastReceiver {

        @Override
        public void onReceive(Context context, Intent intent) {
            onWifiConnectionChanged(isWifiConnected());
        }
    }
}
