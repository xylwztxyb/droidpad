package com.droidpad;

import android.content.Context;
import android.util.DisplayMetrics;
import android.view.WindowManager;

import java.io.IOException;
import java.lang.ref.WeakReference;
import java.net.Socket;
import java.net.SocketTimeoutException;

class Util {

    /**
     * Little ending
     **/
    static byte[] int2Bytes(int i) {
        byte[] bytes = new byte[4];
        bytes[0] = (byte) (i & 0xFF);
        bytes[1] = (byte) (i >>> 8 & 0xFF);
        bytes[2] = (byte) (i >>> 16 & 0xFF);
        bytes[3] = (byte) (i >>> 24 & 0xFF);
        return bytes;
    }

    static <T> T getWeakRefValueOrDie(WeakReference<T> ref) {
        T value = ref.get();
        if (value == null)
            throw new RuntimeException("Ref " + ref.toString() + " has died.");
        return value;
    }

    @SuppressWarnings("unchecked")
    static <T> T typeCast(final T t, Object obj) {
        return (T) obj;
    }

    static Packet pingServer(Socket socket) {
        Packet send = new Packet();
        send.setCmd(Constants.Command.MIME_CMD_PING);
        try
        {
            socket.getOutputStream().write(send._buffer.get());
        } catch (IOException e)
        {
            e.printStackTrace();
            return null;
        }

        Packet rcv = new Packet();
        int size = rcv._buffer.get().length;
        try
        {
            int n = socket.getInputStream().read(rcv._buffer.get());
            if (n >= size)
                return rcv;
            else
                return null;
        } catch (SocketTimeoutException e)
        {
            e.printStackTrace();
            return null;
        } catch (IOException e)
        {
            e.printStackTrace();
            return null;
        }
    }

    static int getDisplayWidth(Context context) {
        int width = 0;
        WindowManager wm = (WindowManager) context.getSystemService(Context.WINDOW_SERVICE);
        if (wm != null)
        {
            DisplayMetrics dm = new DisplayMetrics();
            wm.getDefaultDisplay().getMetrics(dm);
            width = dm.widthPixels;
        }
        return width;
    }
}
