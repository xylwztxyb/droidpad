package com.droidpad;

public interface IDPServiceStateCallback {

    void onPingFromServer(int type);

    void onSocketConnNone();

    void onSocketConnected(int type);

    void onSocketThreadInitError(int type);
}
