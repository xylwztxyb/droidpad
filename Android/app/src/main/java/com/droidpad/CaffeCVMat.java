package com.droidpad;

import android.util.Log;

public class CaffeCVMat {
    private static final String TAG = "CaffeCVMat";

    static final int CF_WIDTH = 227;
    static final int CF_HEIGHT = 227;

    long mMat;//DO NOT DELETE! This is used by native code.

    private CaffeCVMat() {
    }

    static CaffeCVMat create() {
        CaffeCVMat mat = new CaffeCVMat();
        mat.cvMatCreate(CF_WIDTH, CF_HEIGHT);
        return mat;
    }

    private native void cvMatCreate(int width, int height);

    public native void cvMatDestroy();

    public native void cvMatAddPathPoint(int x, int y, boolean newPath);

    public native void cvMatReset();
}
