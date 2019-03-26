package com.droidpad;

import android.os.Environment;

class CaffeEngine {

    private static final String ExternalStoragePath = Environment.getExternalStorageDirectory().getPath();
    private static final String DEPLOY_FILE = ExternalStoragePath + "/droidpad/deploy.prototxt";
    private static final String MODEL_FILE = ExternalStoragePath + "/droidpad/trained.caffemodel";
    private static final String MEAN_FILE = ExternalStoragePath + "/droidpad/mean.binaryproto";
    private static final String LABLE_FILE = ExternalStoragePath + "/droidpad/lable.txt";

    private static CaffeEngine sInstance = null;
    private State mState = State.UNINITED;

    private CaffeEngine() {
    }

    static synchronized CaffeEngine getInstance() {
        if (sInstance == null)
            sInstance = new CaffeEngine();
        return sInstance;
    }

    boolean setup() {
        if (mState == State.INITED)
            return true;
        mState = State.INITING;
        boolean res = caffeSetup(DEPLOY_FILE, MODEL_FILE, MEAN_FILE, LABLE_FILE);
        mState = res ? State.INITED : State.UNINITED;
        return res;
    }

    void tearDown() {
        caffeTearDown();
        sInstance = null;
    }

    State getState() {
        return mState;
    }

    synchronized String predic(CaffeCVMat mat, int max_num) {
        return cfPredic(mat, max_num);
    }

    private native boolean caffeSetup(String deploy_file, String model_file, String mean_file,
                                      String lable_file);

    private native String cfPredic(CaffeCVMat mat, int max_num);

    private native void caffeTearDown();

    public enum State {UNINITED, INITING, INITED}

}
