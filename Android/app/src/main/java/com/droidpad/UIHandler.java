package com.droidpad;

import android.graphics.drawable.AnimationDrawable;
import android.os.Handler;
import android.os.Message;
import android.support.v4.app.Fragment;
import android.support.v4.app.FragmentManager;
import android.support.v4.app.FragmentTransaction;
import android.view.View;
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.Toast;

import java.lang.ref.WeakReference;

class UIHandler extends Handler implements IDPServiceStateCallback,
        WordComposer.OnWordsComposedListener {

    private static final String TAG = UIHandler.class.getSimpleName();

    private static UIHandler sInstance = null;
    private WeakReference<MainActivity> mActivityRef;
    private DroidPadService mDPService;

    private WordComposer mComposer;
    private AnimationDrawable mUSBAnimDrawable;
    private AnimationDrawable mWifiAnimDrawable;


    private Fragment[] mFragments = new Fragment[2];
    private FragmentManager mFragmentManager;

    private int mFragmentType = FragmentType.TYPE_AI;

    private UIHandler(MainActivity activity) {
        mActivityRef = new WeakReference<>(activity);
        if (DroidPadApp.getService() != null)
        {
            mDPService = DroidPadApp.getService();
            mDPService.addServiceStateCallback(this);
        }

        LinearLayout layout = activity.findViewById(R.id.composer);
        mComposer = new WordComposer(activity, layout);
        mComposer.setOnWordsComposedListener(this);

        mUSBAnimDrawable = (AnimationDrawable) activity.getResources().getDrawable(R.drawable.usb_anim, null);
        mWifiAnimDrawable = (AnimationDrawable) activity.getResources().getDrawable(R.drawable.wifi_anim, null);

        mFragments[FragmentType.TYPE_AI] = new AIWriteFragment();
        mFragments[FragmentType.TYPE_IME] = new IMEWriteFragment();
        mFragmentManager = activity.getSupportFragmentManager();
        selectWriteFragment(mFragmentType);

        activity.findViewById(R.id.switch_ime).setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                selectWriteFragment((mFragmentType + 1) % 2);
            }
        });
    }

    synchronized static UIHandler getInstance(MainActivity activity) {
        if (sInstance == null)
            sInstance = new UIHandler(activity);
        return sInstance;
    }

    void destroy() {
        mWifiAnimDrawable.stop();
        mUSBAnimDrawable.stop();
        if (mDPService != null)
            mDPService.removeServiceStateCallback(this);
        removeCallbacksAndMessages(null);
        sInstance = null;
    }

    void selectWriteFragment(int type) {
        mComposer.cancel();
        FragmentTransaction transaction = mFragmentManager.beginTransaction();
        Fragment another = mFragmentManager.findFragmentByTag(String.valueOf((type + 1) % 2));
        if (another != null)
            transaction.detach(another);

        Fragment the_f = mFragmentManager.findFragmentByTag(String.valueOf(type));
        if (the_f != null)
            transaction.attach(the_f);
        else
            transaction.add(R.id.container, mFragments[type], String.valueOf(type));
        transaction.commit();
        mFragmentType = type;
    }

    int getWriteFragmentType() {
        return mFragmentType;
    }

    private ImageView getConnStatus() {
        return Util.getWeakRefValueOrDie(mActivityRef).findViewById(R.id.connect_indicator);
    }

    private void handlePing(int connType) {

        if (connType == Constants.SocketType.TYPE_ADB)
        {
            getConnStatus().setImageDrawable(mUSBAnimDrawable);
            mUSBAnimDrawable.setVisible(true, true);
            mUSBAnimDrawable.start();
        } else if (connType == Constants.SocketType.TYPE_WIFI)
        {
            getConnStatus().setImageDrawable(mWifiAnimDrawable);
            mWifiAnimDrawable.setVisible(true, true);
            mWifiAnimDrawable.start();
        }
    }

    private void handleNewConn(int connType) {
        if (connType == Constants.SocketType.TYPE_ADB)
            getConnStatus().setImageResource(R.drawable.usb);
        else if (connType == Constants.SocketType.TYPE_WIFI)
            getConnStatus().setImageResource(R.drawable.wifi);
    }

    @Override
    public void handleMessage(Message msg) {
        switch (msg.what)
        {
            case Constants.Command.CMD_NEW_COMPOSED_WORDS:
                mDPService.sendWords((String) msg.obj);
                break;
            case Constants.Command.MIME_CMD_PING:
            {
                int connType = msg.arg1;
                handlePing(connType);
                break;
            }
            case Constants.ThreadStatus.STATUS_THREAD_INIT_ERROR:
            {
                MainActivity act = Util.getWeakRefValueOrDie(mActivityRef);
                Toast.makeText(act, "WorkerThread exception", Toast.LENGTH_SHORT).show();
                break;
            }
            case Constants.Command.CMD_NEW_RECOGNIZED_WORDS:
            {
                mComposer.addCharWords(((String) msg.obj).toCharArray());
                break;
            }
            case Constants.Command.CMD_NEW_IME_WORDS:
            {
                mComposer.addStringWords(String.valueOf(msg.obj));
                break;
            }
            case Constants.Command.CMD_NEW_CONNECTION:
                handleNewConn(msg.arg1);
                break;
            case Constants.Command.CMD_CONNECTION_BROKEN:
            {
                getConnStatus().setImageResource(R.drawable.unknown);
                break;
            }
            case Constants.Command.CMD_PANEL_RECO_START:
            {
                WritePanel panel = Util.getWeakRefValueOrDie(
                        Util.typeCast(new WeakReference<WritePanel>(null), msg.obj));
                panel.setEnabled(false);
                break;
            }
            case Constants.Command.CMD_PANEL_RECO_END:
            {
                WritePanel panel = Util.getWeakRefValueOrDie(
                        Util.typeCast(new WeakReference<WritePanel>(null), msg.obj));
                panel.setEnabled(true);
                break;
            }
            default:
                super.handleMessage(msg);
        }
    }

    void setDPService(DroidPadService service) {
        mDPService = service;
        mDPService.addServiceStateCallback(this);
        mDPService.requestCurrentConnState(this);
    }

    @Override
    public void onPingFromServer(int type) {
        Message msg = Message.obtain();
        msg.what = Constants.Command.MIME_CMD_PING;
        msg.arg1 = type;
        sendMessage(msg);
    }

    @Override
    public void onSocketConnNone() {
        Message msg = Message.obtain();
        msg.what = Constants.Command.CMD_CONNECTION_BROKEN;
        sendMessage(msg);
    }

    @Override
    public void onSocketConnected(int type) {
        Message msg = Message.obtain();
        msg.what = Constants.Command.CMD_NEW_CONNECTION;
        msg.arg1 = type;
        sendMessage(msg);
    }

    @Override
    public void onSocketThreadInitError(int type) {
        Message msg = Message.obtain();
        msg.what = Constants.ThreadStatus.STATUS_THREAD_INIT_ERROR;
        msg.arg1 = type;
        sendMessage(msg);
    }

    @Override
    public void onComposedString(String words) {
        Message msg = Message.obtain();
        msg.what = Constants.Command.CMD_NEW_COMPOSED_WORDS;
        msg.obj = words;
        sendMessage(msg);
    }

    private class FragmentType {
        private static final int TYPE_AI = 0;
        private static final int TYPE_IME = 1;
    }


}
