package com.droidpad;

import android.content.Context;
import android.view.LayoutInflater;
import android.view.View;
import android.widget.Button;
import android.widget.LinearLayout;
import android.widget.TextView;

import java.lang.ref.WeakReference;
import java.util.ArrayList;
import java.util.List;

public class WordComposer implements TextView.OnClickListener {

    private static final String TAG = WordComposer.class.getSimpleName();

    private WeakReference<LinearLayout> mContainerRef;
    private LayoutInflater mInflater;
    private int mContainerHeight;

    private List<Character> mWordsList = new ArrayList<>();
    private OnWordsComposedListener mListener = null;


    WordComposer(Context context, LinearLayout container) {
        mContainerRef = new WeakReference<>(container);
        mInflater = LayoutInflater.from(context);
        container.post(new Runnable() {
            @Override
            public void run() {
                mContainerHeight = getLayoutOrDie().getHeight();
            }
        });
    }

    private LinearLayout getLayoutOrDie() {
        LinearLayout layout = mContainerRef.get();
        if (layout != null)
            return layout;
        else
            throw new RuntimeException("Main activity has died");
    }

    void addCharWords(char[] words) {
        if (mWordsList.size() != 0 && mListener != null)
            mListener.onComposedString(mWordsList.get(0).toString());

        mWordsList.clear();
        getLayoutOrDie().removeAllViews();
        for (char w : words)
        {
            mWordsList.add(w);
            createComposeTextView(w, mWordsList.size());
        }
    }

    void addStringWords(String string) {
        getLayoutOrDie().removeAllViews();
        createComposeTextView(string);
        AnimUtil.transUpOutAnim(getLayoutOrDie().getChildAt(0),
                mContainerHeight != 0 ? mContainerHeight : 300);
        if (mListener != null)
            mListener.onComposedString(string);
    }

    public void cancel() {
        mWordsList.clear();
        getLayoutOrDie().removeAllViews();
    }

    void setOnWordsComposedListener(OnWordsComposedListener listener) {
        mListener = listener;
    }

    private void createComposeTextView(char word, int index) {
        LinearLayout ll = (LinearLayout) mInflater.inflate(R.layout.compose_button, getLayoutOrDie(), true);
        Button bt = (Button) ll.getChildAt(ll.getChildCount() - 1);
        bt.setText(String.valueOf(word));
        if (index == 1)
            bt.setBackgroundResource(R.drawable.btn_composer_0);
        else
            bt.setBackgroundResource(R.drawable.btn_composer);
        bt.setOnClickListener(this);
    }

    private void createComposeTextView(String string) {
        LinearLayout ll = (LinearLayout) mInflater.inflate(R.layout.compose_button, getLayoutOrDie(), true);
        Button bt = (Button) ll.getChildAt(ll.getChildCount() - 1);
        bt.setText(string);
    }

    @Override
    public void onClick(View view) {
        if (mListener != null)
            mListener.onComposedString(((TextView) view).getText().toString());
        mWordsList.clear();
        getLayoutOrDie().removeAllViews();
    }

    public interface OnWordsComposedListener {
        void onComposedString(String words);
    }
}
