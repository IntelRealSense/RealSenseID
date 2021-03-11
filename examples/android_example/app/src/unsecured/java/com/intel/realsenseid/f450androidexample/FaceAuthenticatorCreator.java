package com.intel.realsenseid.f450androidexample;

import com.intel.realsenseid.api.AndroidSerialConfig;
import com.intel.realsenseid.api.FaceAuthenticator;

public class FaceAuthenticatorCreator {
    private static final String TAG = "FACreator";

    public FaceAuthenticator Create(AndroidSerialConfig config) {
        return new FaceAuthenticator();
    }
}
