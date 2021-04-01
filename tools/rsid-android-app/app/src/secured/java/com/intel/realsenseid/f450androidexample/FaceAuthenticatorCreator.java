package com.intel.realsenseid.f450androidexample;

import android.util.Log;
import com.intel.realsenseid.api.AndroidSerialConfig;
import com.intel.realsenseid.api.FaceAuthenticator;
import com.intel.realsenseid.api.SignHelper;
import com.intel.realsenseid.api.Status;


public class FaceAuthenticatorCreator {
    private static final String TAG = "FACreator";
    private SignHelper m_signHelper;

    public FaceAuthenticator Create(AndroidSerialConfig config) {
        m_signHelper = new SignHelper();
        FaceAuthenticator faceAuthenticator = new FaceAuthenticator(m_signHelper);
        if (null != faceAuthenticator) {
            Log.d(TAG, "FaceAuthenticator class was created");
            faceAuthenticator.Connect(config);
            Status s = m_signHelper.ExchangeKeys(faceAuthenticator);
            faceAuthenticator.Disconnect();
            if (s != Status.Ok) {
                Log.e(TAG, "Failed keys exchange");
            }
        }
        return faceAuthenticator;
    }
}
