package com.droidpad;

import android.view.View;
import android.view.animation.AccelerateInterpolator;
import android.view.animation.DecelerateInterpolator;
import android.view.animation.TranslateAnimation;

class AnimUtil {

    static void transUpOutAnim(View v, int deltaY) {
        TranslateAnimation transAnim = new TranslateAnimation(0, 0,
                0, 0 - deltaY);
        transAnim.setDuration(100);
        transAnim.setFillAfter(true);
        transAnim.setInterpolator(new AccelerateInterpolator());
        transAnim.setStartOffset(100);
        v.startAnimation(transAnim);
    }

    static void transLeftInAnim(View v, int deltaX) {
        TranslateAnimation transAnim = new TranslateAnimation(0 - deltaX, 0,
                0, 0);
        transAnim.setDuration(200);
        transAnim.setFillAfter(true);
        transAnim.setInterpolator(new DecelerateInterpolator());
        v.startAnimation(transAnim);
    }
}
