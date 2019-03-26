package com.droidpad;

import android.content.Context;
import android.os.Bundle;
import android.os.Message;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;
import android.support.design.widget.FloatingActionButton;
import android.support.v4.app.Fragment;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.view.inputmethod.InputMethodManager;

public class IMEWriteFragment extends Fragment implements IMEFakeView.OnWordsComposedListener, View.OnClickListener {

    private IMEFakeView mFakeView;
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

        FloatingActionButton mActionButton = v.findViewById(R.id.open_ime);
        mActionButton.setOnClickListener(this);
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
}
