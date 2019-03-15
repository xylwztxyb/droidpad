package com.droidpad;

import android.os.Environment;
import android.util.Log;

class CaffeEngine {
    private static final String TAG = "CaffeEngine";

    private static CaffeEngine sInstance = null;

    private boolean inited = false;
    private static final String ExternalStoragePath = Environment.getExternalStorageDirectory().getPath();
    private static final String DEPLOY_FILE = ExternalStoragePath + "/droidpad/deploy.prototxt";
    private static final String MODEL_FILE = ExternalStoragePath + "/droidpad/trained.caffemodel";
    private static final String MEAN_FILE = ExternalStoragePath + "/droidpad/mean.binaryproto";
    private static final String LABLE_FILE = ExternalStoragePath + "/droidpad/lable.txt";

    private CaffeEngine() {
    }

    static synchronized CaffeEngine getInstance() {
        if (sInstance == null)
            sInstance = new CaffeEngine();
        return sInstance;
    }

    synchronized boolean setup() {
        if (!inited)
            return inited = caffeSetup(DEPLOY_FILE, MODEL_FILE, MEAN_FILE, LABLE_FILE);
        return true;
    }

    synchronized void tearDown() {
        caffeTearDown();
        sInstance = null;
    }

    synchronized boolean isInited() {
        return inited;
    }

    synchronized String predic(CaffeCVMat mat, int max_num) {
        return cfPredic(mat, max_num);
    }

    private native boolean caffeSetup(String deploy_file, String model_file, String mean_file,
                                      String lable_file);

    private native String cfPredic(CaffeCVMat mat, int max_num);

    private native void caffeTearDown();

}
