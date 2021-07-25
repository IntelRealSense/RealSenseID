package com.intel.realsenseid.f450androidexample;

import android.annotation.SuppressLint;
import android.app.Activity;
import android.util.Log;
import android.widget.ProgressBar;

import com.intel.realsenseid.api.AndroidSerialConfig;
import com.intel.realsenseid.api.FwUpdater;
import com.intel.realsenseid.api.FwUpdater.UpdatePolicyInfo;
import com.intel.realsenseid.api.Status;
import com.intel.realsenseid.api.StringVector;
import com.intel.realsenseid.impl.UsbCdcConnection;

public class FirmwareUpdateLogic {

    private static final int MIN_WAIT_AFTER_RESTART_MS = 7000;
    private static final int MAX_WAIT_AFTER_RESTART_MS = 30000;
    private static final String TAG = "FirmwareUpdateLogic";
    private static final String OPFW = "OPFW";
    private Activity wrappingActivity;
    private StringVector moduleNames;

    public FirmwareUpdateLogic(Activity activity) {
        this.wrappingActivity = activity;
        this.moduleNames = new StringVector();
    }

    public UpdatePolicyInfo decideUpdatePolicy(String binPath) {
        UsbCdcConnection connection = new UsbCdcConnection();
        if (!connection.FindSupportedDevice(wrappingActivity.getApplicationContext())) {
            throw new RuntimeException("Supported USB device not found");
        }
        if (!connection.OpenConnection()) {
            throw new RuntimeException("Couldn't open connection to USB device");
        }
        FwUpdater.Settings settings = getUpdaterSettings(connection);
        FwUpdater fwu = new FwUpdater();

        UpdatePolicyInfo updatePolicyInfo = fwu.DecideUpdatePolicy(settings, binPath);
        connection.CloseConnection();
        return updatePolicyInfo;
    }

    private class ProgressHandler extends FwUpdater.EventHandler {
        private int m_min, m_max;

        public ProgressHandler(int min, int max) {
            m_min = min;
            m_max = max;
        }

        @SuppressLint("DefaultLocale")
        @Override
        public void OnProgress(float progress) {
            wrappingActivity.runOnUiThread(new Runnable() {
                @Override
                public void run() {
                    ProgressBar pb = (ProgressBar) wrappingActivity.findViewById(R.id.update_progress_progressBar);
                    pb.setProgress((int) (m_min + progress * (m_max - m_min)));
                }
            });
        }
    }

    public void updateFirmware(String path) {
        new Thread(new Runnable() {
            public void run() {
                UsbCdcConnection connection = new UsbCdcConnection();
                if (connection == null) {
                    throw new RuntimeException("Error opening a USB connection");
                }
                if (!connection.FindSupportedDevice(wrappingActivity.getApplicationContext())) {
                    throw new RuntimeException("Supported USB device not found");
                }
                if (!connection.OpenConnection()) {
                    throw new RuntimeException("Couldn't open connection to USB device");
                }
                FwUpdater.Settings settings = getUpdaterSettings(connection);
                FwUpdater fwu = new FwUpdater();
                UpdatePolicyInfo updatePolicyInfo = fwu.DecideUpdatePolicy(settings, path);
                UpdatePolicyInfo.UpdatePolicy policy = updatePolicyInfo.getPolicy();
                if (policy == UpdatePolicyInfo.UpdatePolicy.NOT_ALLOWED || policy == UpdatePolicyInfo.UpdatePolicy.REQUIRE_INTERMEDIATE_FW) {
                    // shouldn't happen because we check it before we call this function.
                    return;
                }
                String[] fwVersion = new String[]{""};
                String[] recognitionVersion = new String[]{""};
                boolean success = fwu.ExtractFwInformation(path, fwVersion, recognitionVersion, moduleNames);
                if (!success) {
                    throw new RuntimeException("Couldn't extract firmware information from binary file");
                }
                Status updateStatus;
                if (policy == UpdatePolicyInfo.UpdatePolicy.CONTINOUS) {
                    FwUpdater.EventHandler handler = new ProgressHandler(0, 100);
                    updateStatus = fwu.UpdateModules(handler, settings, path, moduleNames);
                } else {
                    FwUpdater.EventHandler handler = new ProgressHandler(0, 100 / moduleNames.size());
                    StringVector OPFWVector = new StringVector();
                    OPFWVector.add(OPFW);
                    updateStatus = fwu.UpdateModules(handler, settings, path, OPFWVector);
                }
                connection.CloseConnection();
                if (policy == UpdatePolicyInfo.UpdatePolicy.OPFW_FIRST && updateStatus == Status.Ok) {
                    // To support FW update process that starts in the loader we first only install
                    // OPFW. Then reboot to the new OPFW and then install the rest of the modules.
                    firmwareUpdateSecondGo(path);
                } else {
                    // TODO: Use the returned update status.
                    waitForDevice(connection);
                    wrappingActivity.runOnUiThread(new Runnable() {
                        @Override
                        public void run() {
                            wrappingActivity.finish();
                        }
                    });
                }
            }
        }).start();
    }

    private void firmwareUpdateSecondGo(String path) {
        UsbCdcConnection connection = new UsbCdcConnection();
        waitForDevice(connection);
        connection.RequestDevicePermission(wrappingActivity.getApplicationContext(), new UsbCdcConnection.PermissionCallback() {
            @Override
            public void Response(boolean permissionGranted) {
                if (permissionGranted && connection.OpenConnection()) {
                    BurnModels(path, connection);
                } else {
                    throw new RuntimeException("Couldn't open connection to USB device");
                }
            }
        });
    }

    private void BurnModels(String path, UsbCdcConnection connection)
    {
        new Thread(new Runnable() {
            public void run() {
                FwUpdater.EventHandler handler = new ProgressHandler(100 / 7, 100);
                FwUpdater.Settings settings = getUpdaterSettings(connection);
                FwUpdater fwu = new FwUpdater();
                moduleNames.remove(OPFW);
                Status s = fwu.UpdateModules(handler, settings, path, moduleNames);
                connection.CloseConnection();
                waitForDevice(connection);
                wrappingActivity.runOnUiThread(new Runnable() {
                    @Override
                    public void run () {
                        wrappingActivity.finish();
                    }
                });
            }
        }).start();
    }

    private void waitForDevice(UsbCdcConnection connection) {
        boolean deviceFound = false;
        try {
            Thread.sleep(MIN_WAIT_AFTER_RESTART_MS);
            int sleepIntervalsMs = 1000;
            for (int totalWait = MIN_WAIT_AFTER_RESTART_MS; !deviceFound && totalWait <= MAX_WAIT_AFTER_RESTART_MS; totalWait += sleepIntervalsMs)
            {
                deviceFound = connection.FindSupportedDevice(wrappingActivity.getApplicationContext());
                if (!deviceFound) {
                    Thread.sleep(sleepIntervalsMs);
                }
            }
        } catch (InterruptedException e) {
            Log.e(this.getClass().getName(), "Exception thrown from sleep waiting to restart the device");
        }
        if (!deviceFound) {
            throw new RuntimeException("Supported USB device not found");
        }
    }

    private FwUpdater.Settings getUpdaterSettings(UsbCdcConnection connection) {
        FwUpdater.Settings settings = new FwUpdater.Settings();
        AndroidSerialConfig config = new AndroidSerialConfig();
        config.setFileDescriptor(connection.GetFileDescriptor());
        config.setReadEndpoint(connection.GetReadEndpointAddress());
        config.setWriteEndpoint(connection.GetWriteEndpointAddress());
        settings.setAndroid_config(config);
        return settings;
    }
}
