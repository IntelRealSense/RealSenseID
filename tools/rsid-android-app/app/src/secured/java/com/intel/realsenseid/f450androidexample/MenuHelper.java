package com.intel.realsenseid.f450androidexample;

import android.util.Log;

import com.intel.realsenseid.api.FaceAuthenticator;
import com.intel.realsenseid.api.Status;

public class MenuHelper {
    private static final String TAG = "MenuHelper";

    public static void HandleSelection(int id, FaceAuthenticator faceAuthenticator) {
        switch (id) {
            case R.id.action_unpair:
                Status s = faceAuthenticator.Unpair();
                Log.d(TAG, String.format("Unpair action status: %s", s.toString()));
        }
    }
}
