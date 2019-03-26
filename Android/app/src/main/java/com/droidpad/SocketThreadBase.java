package com.droidpad;

abstract class SocketThreadBase extends Thread {

    private final Object mSuspendLock = new Object();
    private final Object mQuitLock = new Object();
    private volatile boolean bQuit = false;
    private volatile boolean bSuspend = false;

    SocketThreadBase(String name) {
        super(name);
    }

    protected abstract boolean initSocket();

    protected abstract boolean threadLoop();

    protected abstract void onThreadQuit();

    final boolean isSuspended() {
        return bSuspend;
    }

    final void requestSuspend() {
        bSuspend = true;
    }

    final void suspendLocked() {
        synchronized (mSuspendLock)
        {
            bSuspend = true;
            while (bSuspend)
            {
                try
                {
                    mSuspendLock.wait();
                } catch (InterruptedException e)
                {
                    e.printStackTrace();
                }
            }
        }
    }

    final void wakeup() {
        synchronized (mSuspendLock)
        {
            bSuspend = false;
            mSuspendLock.notify();
        }
    }

    final void quit() {
        onThreadQuit();
        bQuit = true;
        synchronized (mSuspendLock)
        {
            if (bSuspend)
            {
                bSuspend = false;
                mSuspendLock.notify();
            }
        }
        try
        {
            join();
        } catch (InterruptedException e)
        {
            e.printStackTrace();
        }
    }

    @Override
    public final void run() {
        while (true)
        {
            synchronized (mSuspendLock)
            {
                while (bSuspend)
                {
                    try
                    {
                        mSuspendLock.wait();
                    } catch (InterruptedException e)
                    {
                        e.printStackTrace();
                    }
                }
            }

            if (bQuit) return;
            if (!threadLoop())
            {
                onThreadQuit();
                return;
            }
        }
    }
}
