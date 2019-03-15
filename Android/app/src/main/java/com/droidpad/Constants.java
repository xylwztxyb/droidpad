package com.droidpad;

public class Constants {

    public class Command {

        static final int CMD_NEW_COMPOSED_WORDS = 50;

        static final int CMD_CONNECTION_BROKEN = 51;

        static final int CMD_NEW_CONNECTION = 52;

        static final int CMD_NEW_IME_WORDS = 53;

        static final int CMD_NEW_RECOGNIZED_WORDS = 54;

        static final int CMD_REQUEST_CURRENT_STATE = 55;

        static final int CMD_USB_CABLE_PLUGOUT = 58;

        static final int CMD_WIFI_POWER_OFF = 59;

        static final int CMD_PANEL_RECO_START = 60;

        static final int CMD_PANEL_RECO_END = 61;

        static final int CMD_APP_EXIT = 62;

        //Defined in WritingPadServer/common.h
        static final int MIME_CMD_PING = 10;

        static final int MIME_CMD_PING_RESPONSE = 19;

        static final int MIME_CMD_REQUEST_ID_FROM_SERVER = 16;

        static final int MIME_CMD_ID_OF_CLIENT = 17;

        static final int MIME_CMD_NEW_WORDS = 2;
    }

    class SocketArg {

        static final int ADB_PORT = 5288;

        static final int ADB_SOCKET_CONNECT_TIMEOUT = 4000;

        static final String ADB_SOCKET_LOOPBACK_ADDRESS = "127.0.0.1";

        static final int WIFI_BRDCST_PORT = 59288;

        static final int WIFI_BRDCST_RECV_TIMEOUT = 2000;

        static final int WIFI_SOCKET_CONNECT_TIMEOUT = 4000;

        static final int SOCKET_TIMEOUT_THRESHOLD = 15;

        static final int SOCKET_SCAN_INTERVAL = 1000;

    }

    class BroadcastCommand {
        //Defined in WritingPadServer/netMonitor.h
        static final int ARE_YOU_MIME_SERVER = 80;

        static final int I_AM_MIME_SERVER = 81;
    }

    class ThreadStatus {
        static final int STATUS_THREAD_INIT_ERROR = 100;
    }

    class SocketType {
        static final int TYPE_ADB = 1;

        static final int TYPE_WIFI = 2;
    }
}
