package com.droidpad;


public interface IRecognizerCallback {
    void onRecognizeStart();
    void onRecognizeEnd();
    void onRecognized(String result);
}
