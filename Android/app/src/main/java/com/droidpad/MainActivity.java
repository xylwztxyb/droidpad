package com.droidpad;

import android.Manifest;
import android.app.AlertDialog;
import android.app.Service;
import android.content.ComponentName;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.ServiceConnection;
import android.content.pm.PackageManager;
import android.os.Bundle;
import android.os.IBinder;
import android.support.constraint.ConstraintLayout;
import android.support.v4.app.ActivityCompat;
import android.support.v4.content.ContextCompat;
import android.support.v7.app.AppCompatActivity;
import android.view.View;
import android.widget.FrameLayout;

import java.util.ArrayList;
import java.util.List;


public class MainActivity extends AppCompatActivity {

    private static final String TAG = "MainActivity";

    private static final String FRAGMENT_TYPE_KEY = "fragment_type";

    private boolean bBackPressed = false;
    private UIHandler mHandler;
    private Thread mThread;

    private FrameLayout mInitLayout;
    private ConstraintLayout mContentLayout;
    private boolean bBoundToService = false;

    private List<OnDPServiceAvailableListener> mListeners = new ArrayList<>();


    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
        checkPermission();

        mInitLayout = findViewById(R.id.init_view);
        mContentLayout = findViewById(R.id.content_view);

        mHandler = UIHandler.getInstance(this);
        mThread = Thread.currentThread();

        DroidPadService service = DroidPadApp.getService();
        if (service == null)
        {
            updateUI(false);
            Intent i = new Intent(this, DroidPadService.class);
            startService(i);
            bindService(i, mConn, Service.BIND_AUTO_CREATE);
            bBoundToService = true;
        } else
        {
            if (!service.isActive())
                service.active();
            updateUI(true);
            service.requestCurrentConnState(mHandler);
        }
    }

    @Override
    protected void onDestroy() {
        mHandler.destroy();
        if (bBackPressed && DroidPadApp.getService() != null)
            DroidPadApp.getService().deActive();
        if (bBoundToService)
            unbindService(mConn);
        super.onDestroy();
    }

    @Override
    public void onBackPressed() {
        bBackPressed = true;
        super.onBackPressed();
    }

    @Override
    protected void onSaveInstanceState(Bundle outState) {
        outState.putInt(FRAGMENT_TYPE_KEY, mHandler.getWriteFragmentType());
        super.onSaveInstanceState(outState);
    }

    @Override
    protected void onRestoreInstanceState(Bundle savedInstanceState) {
        int type = savedInstanceState.getInt(FRAGMENT_TYPE_KEY, 0);
        mHandler.selectWriteFragment(type);
        super.onRestoreInstanceState(savedInstanceState);
    }

    private void onServiceAvailable(DroidPadService service) {
        updateUI(true);
        if (!service.isActive())
            service.active();
        mHandler.setDPService(service);
        for (OnDPServiceAvailableListener listener : mListeners)
            listener.onDPServiceAvailable(service);
    }

    private void updateUI(final boolean serviceAvailable) {
        if (Thread.currentThread() == mThread)
        {
            mContentLayout.setVisibility(serviceAvailable ? View.VISIBLE : View.GONE);
            mInitLayout.setVisibility(serviceAvailable ? View.GONE : View.VISIBLE);
        } else
        {
            runOnUiThread(new Runnable() {
                @Override
                public void run() {
                    mContentLayout.setVisibility(serviceAvailable ? View.VISIBLE : View.GONE);
                    mInitLayout.setVisibility(serviceAvailable ? View.GONE : View.VISIBLE);
                }
            });
        }
    }


    private ServiceConnection mConn = new ServiceConnection() {
        @Override
        public void onServiceConnected(ComponentName componentName, IBinder iBinder) {
            DroidPadService service = ((DroidPadService.DroidPadServiceBinder) iBinder).getService();
            if (service.isRecognizeEngineInited())
            {
                ((DroidPadApp) getApplication()).globalInit(service);
                onServiceAvailable(service);
            } else
            {
                service.setEngineStateCallback(mEngineCallback);
                service.initRecognizeEngine();
            }
        }

        @Override
        public void onServiceDisconnected(ComponentName componentName) {

        }
    };

    private IDPEngineStateCallback mEngineCallback = new IDPEngineStateCallback() {
        @Override
        public void onRecognizeEngineReady(DroidPadService engine) {
            ((DroidPadApp) getApplication()).globalInit(engine);
            onServiceAvailable(engine);
        }

        @Override
        public void onRecognizeEngineError() {
            AlertDialog.Builder builder = new AlertDialog.Builder(MainActivity.this);
            builder.setMessage(R.string.engine_init_error)
                    .setOnDismissListener(new DialogInterface.OnDismissListener() {
                        @Override
                        public void onDismiss(DialogInterface dialogInterface) {
                            MainActivity.this.finish();
                        }
                    }).show();
        }
    };

    void registerOnDPServiceAvailableListener(OnDPServiceAvailableListener listener) {
        mListeners.add(listener);
    }

    void unregisterOnDPServiceAvailableListener(OnDPServiceAvailableListener listener) {
        mListeners.remove(listener);
    }

    private void checkPermission() {
        if (ContextCompat.checkSelfPermission(this,
                Manifest.permission.WRITE_EXTERNAL_STORAGE) != PackageManager.PERMISSION_GRANTED)
        {
            if (ActivityCompat.shouldShowRequestPermissionRationale(this,
                    Manifest.permission.WRITE_EXTERNAL_STORAGE))
            {
            } else
            {
                ActivityCompat.requestPermissions(this,
                        new String[]{Manifest.permission.WRITE_EXTERNAL_STORAGE}, 1);
            }
        }
    }

    public interface OnDPServiceAvailableListener {
        void onDPServiceAvailable(DroidPadService service);
    }
}
