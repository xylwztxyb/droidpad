package com.droidpad;

import android.app.Application;

public class DroidPadApp extends Application {

    static
    {
        System.loadLibrary("droidpad");
    }

    public static final int ID = (int) (Math.random() * 100000);

    private static DroidPadService mService = null;

    synchronized static DroidPadService getService() {
        return mService;
    }

    synchronized void globalInit(DroidPadService service) {
        mService = service;
    }

    @Override
    public void onTerminate() {
        mService = null;
        super.onTerminate();
    }
}
