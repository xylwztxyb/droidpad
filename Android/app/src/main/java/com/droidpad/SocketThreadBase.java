package com.droidpad;

abstract class SocketThreadBase extends Thread {

    private boolean bQuit = false;

    private boolean bSuspend = false;
    private final Object mSuspendLock = new Object();
    private final Object mQuitLock = new Object();

    SocketThreadBase(String name) {
        super(name);
    }

    protected abstract boolean initSocket();

    protected abstract boolean threadLoop();

    protected abstract void onThreadQuit();

    final boolean isSuspended() {
        synchronized (mSuspendLock)
        {
            return bSuspend;
        }
    }

    final void requestSuspend() {
        synchronized (mSuspendLock)
        {
            bSuspend = true;
        }
    }

    final void suspendLock() {
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
        synchronized (mQuitLock)
        {
            bQuit = true;
        }
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

            synchronized (mQuitLock)
            {
                if (bQuit) return;
            }
            if (!threadLoop())
            {
                onThreadQuit();
                return;
            }
        }
    }
}
