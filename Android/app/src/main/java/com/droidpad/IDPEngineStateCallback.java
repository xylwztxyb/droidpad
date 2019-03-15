package com.droidpad;

public interface IDPEngineStateCallback {
    
    void onRecognizeEngineReady(DroidPadService engine);

    void onRecognizeEngineError();
}
