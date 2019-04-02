package com.droidpad;

import android.app.Activity;
import android.content.Context;
import android.graphics.Rect;
import android.os.Bundle;
import android.os.Message;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;
import android.support.design.widget.FloatingActionButton;
import android.support.v4.app.Fragment;
import android.util.DisplayMetrics;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.view.ViewTreeObserver;
import android.view.inputmethod.InputMethodManager;

public class IMEWriteFragment extends Fragment implements IMEFakeView.OnWordsComposedListener,
        View.OnClickListener,
        ViewTreeObserver.OnGlobalLayoutListener {

    private IMEFakeView mFakeView;
    private FloatingActionButton mFloatButton;
    private InputMethodManager imm;
    private UIHandler mUIHandler;

    public IMEWriteFragment() {
        super();
    }

    @Override
    public void onCreate(@Nullable Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        imm = (InputMethodManager) requireContext()
                .getSystemService(Context.INPUT_METHOD_SERVICE);
    }

    @Nullable
    @Override
    public View onCreateView(@NonNull LayoutInflater inflater, @Nullable ViewGroup container, Bundle savedInstanceState) {
        View v = inflater.inflate(R.layout.fragment_ime_write, container, false);
        mFakeView = v.findViewById(R.id.fake_view);
        mFakeView.setOnWordsComposedListener(this);
        mFakeView.requestFocus();

        mFloatButton = v.findViewById(R.id.open_ime);
        mFloatButton.setOnClickListener(this);
        return v;
    }

    @Override
    public void onActivityCreated(@Nullable Bundle savedInstanceState) {
        super.onActivityCreated(savedInstanceState);
        MainActivity act = (MainActivity) getActivity();
        if (act != null)
            mUIHandler = UIHandler.getInstance(act);
    }

    @Override
    public void onResume() {
        super.onResume();
        AnimUtil.transLeftInAnim(getView(), Util.getDisplayWidth(getContext()));
        if (imm != null)
            imm.showSoftInput(mFakeView, InputMethodManager.SHOW_FORCED);
        mFloatButton.getViewTreeObserver().addOnGlobalLayoutListener(this);
    }

    @Override
    public void onPause() {
        super.onPause();
        if (imm != null)
            imm.hideSoftInputFromWindow(mFakeView.getWindowToken(), 0);
    }

    @Override
    public void onWordsComposed(CharSequence sequence) {
        if (mUIHandler != null)
        {
            Message msg = Message.obtain();
            msg.what = Constants.Command.CMD_NEW_IME_WORDS;
            msg.obj = sequence;
            mUIHandler.sendMessage(msg);
        }
    }

    @Override
    public void onClick(View view) {
        if (imm != null)
            imm.showSoftInput(mFakeView, InputMethodManager.SHOW_FORCED);
    }

    @Override
    public void onGlobalLayout() {
        Activity act = getActivity();
        if (act != null)
        {
            Rect rect = new Rect();
            act.getWindow().getDecorView().getWindowVisibleDisplayFrame(rect);
            DisplayMetrics dm = new DisplayMetrics();
            act.getWindowManager().getDefaultDisplay().getMetrics(dm);
            float f = (float) rect.bottom / dm.heightPixels;
            if (f >= 0.9)
                onInputWindowHide();
            else if (f < 0.8)
                onInputWindowShow();
        }
    }

    void onInputWindowHide() {
        mUIHandler.post(new Runnable() {
            @Override
            public void run() {
                mFloatButton.setVisibility(View.VISIBLE);
            }
        });

    }

    void onInputWindowShow() {
        mFloatButton.setVisibility(View.GONE);
    }
}
