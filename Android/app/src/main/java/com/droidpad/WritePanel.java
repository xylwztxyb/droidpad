package com.droidpad;

import android.content.Context;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.DashPathEffect;
import android.graphics.Paint;
import android.graphics.Path;
import android.os.Message;
import android.support.annotation.Nullable;
import android.util.AttributeSet;
import android.view.MotionEvent;
import android.view.View;
import android.widget.Toast;

import java.lang.ref.WeakReference;
import java.util.ArrayList;
import java.util.List;
import java.util.concurrent.locks.ReentrantLock;

public class WritePanel extends View implements IRecognizerCallback, MainActivity.OnDPServiceAvailableListener {
    private static final String TAG = WritePanel.class.getSimpleName();
    private final ReentrantLock mLock = new ReentrantLock();
    private CaffeRecognizer mRecognizer;
    private List<Path> mPathList;
    private Path mCurPath;
    private Paint mPathPaint;
    private Paint mBackgrdPaint;
    private UIHandler mUIHandler;
    private Context mContext;
    private boolean bActionDownPerformed = true;


    public WritePanel(Context context, @Nullable AttributeSet attrs) {
        super(context, attrs);

        mContext = context;
        if (DroidPadApp.getService() == null)
        {
            ((MainActivity) context).registerOnDPServiceAvailableListener(this);
            setEnabled(false);
        } else
            updateEnv(DroidPadApp.getService());

        setLayerType(View.LAYER_TYPE_SOFTWARE, null);

        mPathList = new ArrayList<>();
        mPathPaint = new Paint();
        mPathPaint.setColor(Color.BLACK);
        mPathPaint.setStyle(Paint.Style.STROKE);
        mPathPaint.setStrokeWidth(5);

        mBackgrdPaint = new Paint();
        mBackgrdPaint.setColor(getResources().getColor(R.color.colorLightBlue));
        mBackgrdPaint.setStyle(Paint.Style.STROKE);
        mBackgrdPaint.setStrokeWidth(3);
        mBackgrdPaint.setPathEffect(new DashPathEffect(new float[]{5, 10}, 0));
    }

    public void clear() {
        mPathList.clear();
        mCurPath.reset();
        postInvalidate();
    }

    @Override
    protected void onAttachedToWindow() {
        super.onAttachedToWindow();
        mUIHandler = UIHandler.getInstance((MainActivity) mContext);
    }

    @Override
    protected void onDetachedFromWindow() {
        super.onDetachedFromWindow();
        if (mRecognizer != null)
            mRecognizer.shutdown();
        mRecognizer = null;
        ((MainActivity) mContext).unregisterOnDPServiceAvailableListener(this);
    }

    @Override
    protected void onDraw(Canvas canvas) {
        drawBackground(canvas);
        for (Path path : mPathList)
        {
            canvas.drawPath(path, mPathPaint);
        }
        if (mCurPath != null && !mCurPath.isEmpty())
            canvas.drawPath(mCurPath, mPathPaint);
    }

    private void drawBackground(Canvas canvas) {
        float startX = 0;
        float startY = getHeight() / 2;
        float endX = startX + getWidth();
        float endY = startY;
        canvas.drawLine(startX, startY, endX, endY, mBackgrdPaint);

        startX = getWidth() / 2;
        startY = 0;
        endX = startX;
        endY = getHeight();
        canvas.drawLine(startX, startY, endX, endY, mBackgrdPaint);
    }

    @Override
    public boolean onTouchEvent(MotionEvent event) {
        if (!mLock.tryLock() || !isEnabled())
            return true;
        try
        {
            switch (event.getAction() & MotionEvent.ACTION_MASK)
            {
                case MotionEvent.ACTION_DOWN:
                {
                    bActionDownPerformed = true;
                    mCurPath = new Path();
                    mCurPath.moveTo(event.getX(0), event.getY(0));
                    if (mRecognizer != null)
                        mRecognizer.onTouchEvent(this, event);
                    break;
                }
                case MotionEvent.ACTION_MOVE:
                {
                    if (bActionDownPerformed)
                    {
                        mCurPath.lineTo(event.getX(0), event.getY(0));
                        if (mRecognizer != null)
                            mRecognizer.onTouchEvent(this, event);
                        invalidate();
                    }
                    break;
                }
                case MotionEvent.ACTION_UP:
                {
                    if (bActionDownPerformed)
                    {
                        bActionDownPerformed = false;
                        mPathList.add(mCurPath);
                        if (mRecognizer != null)
                            mRecognizer.onTouchEvent(this, event);
                        invalidate();
                    }
                    break;
                }
            }
            return true;
        } finally
        {
            mLock.unlock();
        }
    }

    private void updateEnv(DroidPadService service) {
        if (mRecognizer == null)
        {
            mRecognizer = new CaffeRecognizer(service);
            if (mRecognizer.setup())
            {
                mRecognizer.setRecognizerCallback(this);
                this.setEnabled(true);
            } else
                Toast.makeText(getContext(), "Caffe setup failed", Toast.LENGTH_SHORT).show();
        }
    }

    @Override
    public void onRecognizeStart() {
        mLock.lock();
        Message msg = Message.obtain();
        msg.what = Constants.Command.CMD_PANEL_RECO_START;
        msg.obj = new WeakReference<>(this);
        mUIHandler.sendMessage(msg);
    }

    @Override
    public void onRecognizeEnd() {
        clear();
        mLock.unlock();
        Message msg = Message.obtain();
        msg.what = Constants.Command.CMD_PANEL_RECO_END;
        msg.obj = new WeakReference<>(this);
        mUIHandler.sendMessage(msg);
    }

    @Override
    public void onRecognized(String result) {
        Message msg = Message.obtain();
        msg.what = Constants.Command.CMD_NEW_RECOGNIZED_WORDS;
        msg.obj = result;
        mUIHandler.sendMessage(msg);
    }

    @Override
    public void onDPServiceAvailable(DroidPadService service) {
        updateEnv(service);
    }
}
