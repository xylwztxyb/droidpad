package com.droidpad;

import android.app.Service;
import android.content.Intent;
import android.os.Binder;
import android.os.IBinder;
import android.os.Message;
import android.support.annotation.NonNull;
import android.util.Log;

public class DroidPadService extends Service {

    private static final String TAG = DroidPadService.class.getSimpleName();

    private DPServiceHandler mHandler;
    private boolean bActive = false;

    private CaffeEngine mEngine;
    private IDPEngineStateCallback mEngineStateCallback;
    private DroidPadServiceBinder mBinder;

    private AdbSocketProducer mAdbSockProducer;
    private WifiSocketProducer mWifiSockProducer;

    public DroidPadService() {
    }

    @Override
    public IBinder onBind(Intent intent) {
        return mBinder;
    }

    @Override
    public void onCreate() {
        super.onCreate();
        mHandler = DPServiceHandler.create();
        mBinder = new DroidPadServiceBinder();
    }

    @Override
    public int onStartCommand(Intent intent, int flags, int startId) {
        return super.onStartCommand(intent, flags, startId);
    }

    @Override
    public void onDestroy() {
        if (mEngine.getState() == CaffeEngine.State.INITED)
            mEngine.tearDown();
        mHandler.destroy();
        super.onDestroy();
    }

    void initRecognizeEngine() {
        mEngine = CaffeEngine.getInstance();
        CaffeEngine.State state = mEngine.getState();
        if (state == CaffeEngine.State.INITED || state == CaffeEngine.State.INITING)
            return;

        new Thread(new Runnable() {
            @Override
            public void run() {
                if (!mEngine.setup())
                {
                    if (mEngineStateCallback != null)
                        mEngineStateCallback.onRecognizeEngineError();
                } else
                {
                    if (mEngineStateCallback != null)
                        mEngineStateCallback.onRecognizeEngineReady(DroidPadService.this);
                }
            }
        }).start();
    }

    boolean isRecognizeEngineInited() {
        return mEngine != null && mEngine.getState() == CaffeEngine.State.INITED;
    }

    /**
     * Predic the CVMat.
     *
     * @param mat, Must not null.
     */
    String predic(@NonNull CaffeCVMat mat, int max_num) {
        return mEngine.predic(mat, max_num);
    }

    void sendWords(String words) {
        Message msg = Message.obtain();
        msg.what = Constants.Command.CMD_NEW_COMPOSED_WORDS;
        msg.obj = words;
        mHandler.sendMessage(msg);
    }


    void addServiceStateCallback(IDPServiceStateCallback callback) {
        mHandler.addDPServiceStateCallback(callback);
    }

    void removeServiceStateCallback(IDPServiceStateCallback callback) {
        mHandler.removeDPServiceStateCallback(callback);
    }

    void setEngineStateCallback(IDPEngineStateCallback callback) {
        mEngineStateCallback = callback;
    }

    void active() {
        if (mAdbSockProducer != null)//A active operation has been under running.
            return;
        mAdbSockProducer = new AdbSocketProducer(this, mHandler);
        mAdbSockProducer.start();
        mWifiSockProducer = new WifiSocketProducer(this, mHandler);
        if (mWifiSockProducer.initSocket())
            mWifiSockProducer.start();
        else
            throw new RuntimeException("Fail to init wifi socket");
        bActive = true;
    }

    boolean isActive() {
        return bActive;
    }

    void deActive() {
        mAdbSockProducer.quit();
        mAdbSockProducer = null;
        mWifiSockProducer.quit();
        mWifiSockProducer = null;
        bActive = false;
        mHandler.sendEmptyMessage(Constants.Command.CMD_APP_EXIT);
    }

    void requestCurrentConnState(IDPServiceStateCallback caller) {
        Message msg = Message.obtain();
        msg.what = Constants.Command.CMD_REQUEST_CURRENT_STATE;
        msg.obj = caller;
        mHandler.sendMessage(msg);
    }

    class DroidPadServiceBinder extends Binder {
        DroidPadService getService() {
            return DroidPadService.this;
        }
    }
}
