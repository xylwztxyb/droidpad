package com.droidpad;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.os.BatteryManager;
import android.os.Handler;
import android.os.HandlerThread;
import android.os.Message;

import java.io.IOException;
import java.net.InetSocketAddress;
import java.net.Socket;
import java.net.SocketException;

public class AdbSocketProducer extends HandlerThread implements IDPServiceStateCallback {

    private static final String TAG = AdbSocketProducer.class.getSimpleName();

    private DPServiceHandler mServiceHandler;
    private Handler mHandler;
    private Context mContext;

    private volatile boolean bAppHasConnection = false;
    private volatile boolean bQuit = false;

    private InetSocketAddress mAdbSocketAddr =
            new InetSocketAddress(Constants.SocketArg.ADB_SOCKET_LOOPBACK_ADDRESS,
                    Constants.SocketArg.ADB_PORT);
    private BatteryBroadcastReceiver mBatteryReceiver;
    private Runnable mScanTask = new Runnable() {
        @Override
        public void run() {
            tryConnect();
        }
    };

    AdbSocketProducer(Context context, DPServiceHandler handler) {
        super(AdbSocketProducer.class.getSimpleName());
        mContext = context;
        mServiceHandler = handler;
        mServiceHandler.addDPServiceStateCallback(this);

        mBatteryReceiver = new BatteryBroadcastReceiver();
        IntentFilter filter = new IntentFilter();
        filter.addAction(Intent.ACTION_POWER_CONNECTED);
        filter.addAction(Intent.ACTION_POWER_DISCONNECTED);
        mContext.registerReceiver(mBatteryReceiver, filter);
    }

    private void tryConnect() {
        Socket socket = new Socket();
        try
        {
            socket.setReuseAddress(true);
            socket.connect(mAdbSocketAddr, Constants.SocketArg.ADB_SOCKET_CONNECT_TIMEOUT);
        } catch (SocketException e)
        {
            //e.printStackTrace();
            closeSocket(socket);
            schedualScan();
            return;
        } catch (IOException e)
        {
            //e.printStackTrace();
            closeSocket(socket);
            schedualScan();
            return;
        }

        if (Util.pingServer(socket) != null)
        {
            mHandler.removeCallbacksAndMessages(null);
            handleNewSocket(socket);
        } else
            schedualScan();
    }

    private synchronized void schedualScan() {
        if (!bQuit && mHandler != null && isUSBPowerConnected() && !bAppHasConnection)
        {
            mHandler.removeCallbacksAndMessages(null);
            mHandler.postDelayed(mScanTask, Constants.SocketArg.SOCKET_SCAN_INTERVAL);
        }
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

    private void handleNewSocket(Socket socket) {
        Message msg = Message.obtain();
        msg.what = Constants.Command.CMD_NEW_CONNECTION;
        msg.obj = socket;
        msg.arg1 = Constants.SocketType.TYPE_ADB;
        mServiceHandler.sendMessage(msg);
    }

    private boolean isUSBPowerConnected() {
        IntentFilter filter = new IntentFilter(Intent.ACTION_BATTERY_CHANGED);
        Intent i = mContext.registerReceiver(null, filter);
        boolean ret = false;
        if (i != null)
        {
            int source = i.getIntExtra(BatteryManager.EXTRA_PLUGGED, -1);
            ret = source == BatteryManager.BATTERY_PLUGGED_USB;
        }
        return ret;
    }

    @Override
    public synchronized boolean quit() {
        bQuit = true;
        mContext.unregisterReceiver(mBatteryReceiver);
        mServiceHandler.removeDPServiceStateCallback(this);
        mHandler.removeCallbacksAndMessages(null);
        return super.quit();
    }

    @Override
    protected void onLooperPrepared() {
        mHandler = new Handler();
        schedualScan();
    }

    @Override
    public void onPingFromServer(int type) {
    }

    @Override
    public void onSocketConnNone() {
        bAppHasConnection = false;
        schedualScan();
    }

    @Override
    public void onSocketConnected(int type) {
        bAppHasConnection = true;
        mHandler.removeCallbacksAndMessages(null);
    }

    @Override
    public void onSocketThreadInitError(int type) {

    }

    private void onUsbPlugStateChanged(boolean pluged) {
        if (!pluged)
        {
            mHandler.removeCallbacksAndMessages(null);
            mServiceHandler.sendEmptyMessage(Constants.Command.CMD_USB_CABLE_PLUGOUT);
        } else
            schedualScan();
    }

    private class BatteryBroadcastReceiver extends BroadcastReceiver {

        @Override
        public void onReceive(Context context, Intent intent) {
            String action = intent.getAction();
            if (Intent.ACTION_POWER_CONNECTED.equals(action) && isUSBPowerConnected())
                onUsbPlugStateChanged(true);
            else if (Intent.ACTION_POWER_DISCONNECTED.equals(action))
                onUsbPlugStateChanged(false);
        }
    }
}
