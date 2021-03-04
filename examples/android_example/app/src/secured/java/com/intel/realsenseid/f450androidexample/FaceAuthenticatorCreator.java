package com.intel.realsenseid.f450androidexample;

import android.util.Log;
import com.intel.realsenseid.api.FaceAuthenticator;
import com.intel.realsenseid.api.SignHelper;
import com.intel.realsenseid.api.Status;
import com.intel.realsenseid.impl.UsbCdcConnection;


public class FaceAuthenticatorCreator {
    private static final String TAG = "FACreator";
    private SignHelper m_signHelper;

    public FaceAuthenticator Create(UsbCdcConnection connection) {
        m_signHelper = new SignHelper();
        FaceAuthenticator faceAuthenticator = new FaceAuthenticator(m_signHelper);
        if (null != faceAuthenticator) {
            Log.d(TAG, "FaceAuthenticator class was created");
            faceAuthenticator.Connect(
                    connection.GetFileDescriptor(),
                    connection.GetReadEndpointAddress(),
                    connection.GetWriteEndpointAddress());
            Status s = m_signHelper.ExchangeKeys(faceAuthenticator);
            faceAuthenticator.Disconnect();
            if (s != Status.Ok) {
                Log.e(TAG, "Failed keys exchange");
            }
        }
        return faceAuthenticator;
    }
}
