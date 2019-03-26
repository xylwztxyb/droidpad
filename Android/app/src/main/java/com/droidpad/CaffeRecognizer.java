package com.droidpad;

import android.view.MotionEvent;
import android.view.View;

import java.util.Timer;
import java.util.TimerTask;

public class CaffeRecognizer {

    private static final String TAG = "CaffeRecognizer";

    private Timer mTimer;
    private TimerTask mTask;
    private CaffeCVMat mCVMat;
    private volatile boolean bTaskCompleted = true;

    private DroidPadService mCaffeService;
    private IRecognizerCallback mCallback = null;

    CaffeRecognizer(DroidPadService service) {
        mCaffeService = service;
        mTimer = new Timer(true);
        mTask = new TimerTask() {
            @Override
            public void run() {
                //let it empty
            }
        };
    }

    public boolean setup() {
        mCVMat = CaffeCVMat.create();
        return true;
    }

    public void shutdown() {
        if (!mTask.cancel())
            waitForTaskEnd();
        mTimer.cancel();
        mCVMat.cvMatDestroy();
        mCVMat = null;
        mCallback = null;
    }

    private synchronized void waitForTaskEnd() {
        while (!bTaskCompleted)
        {
            try
            {
                wait();
            } catch (InterruptedException e)
            {
                e.printStackTrace();
            }
        }
    }

    void setRecognizerCallback(IRecognizerCallback callback) {
        mCallback = callback;
    }

    void onTouchEvent(View v, MotionEvent event) {

        int v_width = v.getWidth();
        int v_height = v.getHeight();

        switch (event.getAction() & MotionEvent.ACTION_MASK)
        {
            case MotionEvent.ACTION_DOWN:
            {
                mTask.cancel();
                mTimer.purge();

                float act_x = event.getX(0);
                float act_y = event.getY(0);
                int cf_x = (int) (act_x / v_width * CaffeCVMat.CF_WIDTH);
                int cf_y = (int) (act_y / v_height * CaffeCVMat.CF_HEIGHT);
                mCVMat.cvMatAddPathPoint(cf_x, cf_y, true);
                break;
            }
            case MotionEvent.ACTION_MOVE:
            {
                float act_x = event.getX(0);
                float act_y = event.getY(0);
                int cf_x = (int) (act_x / v_width * CaffeCVMat.CF_WIDTH);
                int cf_y = (int) (act_y / v_height * CaffeCVMat.CF_HEIGHT);
                mCVMat.cvMatAddPathPoint(cf_x, cf_y, false);
                //Log.i(TAG, "AddPoint: " + cf_x + "," + cf_y);
                break;
            }
            case MotionEvent.ACTION_UP:
            {
                mTask = new TimerTask() {
                    @Override
                    public void run() {
                        bTaskCompleted = false;
                        if (mCallback != null)
                        {
                            String result;
                            try
                            {
                                mCallback.onRecognizeStart();
                                result = mCaffeService.predic(mCVMat, 3);
                                mCVMat.cvMatReset();
                            } finally
                            {
                                mCallback.onRecognizeEnd();
                            }
                            if (result != null && result.length() > 0)
                                mCallback.onRecognized(result);
                        }
                        bTaskCompleted = true;
                        synchronized (CaffeRecognizer.this)
                        {

                            CaffeRecognizer.this.notify();
                        }
                    }
                };
                mTimer.schedule(mTask, 300);
                break;
            }
        }
    }
}
