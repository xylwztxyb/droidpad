package com.droidpad;

import android.content.Context;
import android.os.Bundle;
import android.os.Handler;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;
import android.util.AttributeSet;
import android.util.Log;
import android.view.KeyEvent;
import android.view.View;
import android.view.inputmethod.CompletionInfo;
import android.view.inputmethod.CorrectionInfo;
import android.view.inputmethod.EditorInfo;
import android.view.inputmethod.ExtractedText;
import android.view.inputmethod.ExtractedTextRequest;
import android.view.inputmethod.InputConnection;
import android.view.inputmethod.InputContentInfo;

class IMEFakeView extends View {

    private static final String TAG = "IMEFakeView";

    private OnWordsComposedListener mListener;

    public IMEFakeView(Context context, @Nullable AttributeSet attrs) {
        super(context, attrs);
    }

    @Override
    public InputConnection onCreateInputConnection(EditorInfo outAttrs) {
        return mInputConn;
    }

    void setOnWordsComposedListener(OnWordsComposedListener listener) {
        mListener = listener;
    }

    private InputConnection mInputConn = new InputConnection() {
        @Override
        public CharSequence getTextBeforeCursor(int i, int i1) {
            return null;
        }

        @Override
        public CharSequence getTextAfterCursor(int i, int i1) {
            return null;
        }

        @Override
        public CharSequence getSelectedText(int i) {
            return null;
        }

        @Override
        public int getCursorCapsMode(int i) {
            return 0;
        }

        @Override
        public ExtractedText getExtractedText(ExtractedTextRequest extractedTextRequest, int i) {
            return null;
        }

        @Override
        public boolean deleteSurroundingText(int i, int i1) {
            return false;
        }

        @Override
        public boolean deleteSurroundingTextInCodePoints(int i, int i1) {
            return false;
        }

        @Override
        public boolean setComposingText(CharSequence charSequence, int i) {
            return false;
        }

        @Override
        public boolean setComposingRegion(int i, int i1) {
            return false;
        }

        @Override
        public boolean finishComposingText() {
            return false;
        }

        @Override
        public boolean commitText(CharSequence charSequence, int i) {
            Log.i(TAG, "commitText: " + charSequence);
            if (mListener != null)
                mListener.onWordsComposed(charSequence);
            return true;
        }

        @Override
        public boolean commitCompletion(CompletionInfo completionInfo) {
            return false;
        }

        @Override
        public boolean commitCorrection(CorrectionInfo correctionInfo) {
            return false;
        }

        @Override
        public boolean setSelection(int i, int i1) {
            return false;
        }

        @Override
        public boolean performEditorAction(int i) {
            return false;
        }

        @Override
        public boolean performContextMenuAction(int i) {
            return false;
        }

        @Override
        public boolean beginBatchEdit() {
            return false;
        }

        @Override
        public boolean endBatchEdit() {
            return false;
        }

        @Override
        public boolean sendKeyEvent(KeyEvent keyEvent) {
            return false;
        }

        @Override
        public boolean clearMetaKeyStates(int i) {
            return false;
        }

        @Override
        public boolean reportFullscreenMode(boolean b) {
            return false;
        }

        @Override
        public boolean performPrivateCommand(String s, Bundle bundle) {
            return false;
        }

        @Override
        public boolean requestCursorUpdates(int i) {
            return false;
        }

        @Override
        public Handler getHandler() {
            return null;
        }

        @Override
        public void closeConnection() {

        }

        @Override
        public boolean commitContent(@NonNull InputContentInfo inputContentInfo, int i, @Nullable Bundle bundle) {
            return false;
        }
    };

    interface OnWordsComposedListener {
        void onWordsComposed(CharSequence sequence);
    }
}
