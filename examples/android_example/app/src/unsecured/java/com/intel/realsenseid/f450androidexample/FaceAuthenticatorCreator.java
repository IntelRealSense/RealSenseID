package com.intel.realsenseid.f450androidexample;

import com.intel.realsenseid.api.FaceAuthenticator;
import com.intel.realsenseid.api.SignatureCallback;
import com.intel.realsenseid.impl.UsbCdcConnection;

public class FaceAuthenticatorCreator {
    private static final String TAG = "FACreator";

    public FaceAuthenticator Create(UsbCdcConnection connection) {
        return new FaceAuthenticator(new SignatureCallback());
    }
}
