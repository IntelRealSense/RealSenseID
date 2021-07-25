package com.intel.realsenseid.f450androidexample;

import android.os.Bundle;
import android.os.Environment;

import com.intel.realsenseid.api.DeviceController;
import com.intel.realsenseid.api.FwUpdater;
import com.intel.realsenseid.api.Status;
import com.intel.realsenseid.api.StringVector;
import com.intel.realsenseid.api.SwigWrapper;
import com.intel.realsenseid.impl.UsbCdcConnection;

import org.jetbrains.annotations.NotNull;

import java.util.regex.Pattern;

import androidx.appcompat.app.AlertDialog;
import androidx.appcompat.app.AppCompatActivity;

public class FilesListActivity extends AppCompatActivity implements FilesListFragment.OnViewItemClickListener {

    private FirmwareUpdateLogic firmwareUpdateLogic;
    private String deviceSerialNumber = null;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        setContentView(R.layout.files_list_activity);
        if (savedInstanceState == null) {
            getSupportFragmentManager().beginTransaction()
                    .add(R.id.container, new FilesListFragment(Environment.getExternalStorageDirectory().getAbsolutePath()))
                    .addToBackStack(Environment.getExternalStorageDirectory().getAbsolutePath())
                    .commit();
        }
        firmwareUpdateLogic = new FirmwareUpdateLogic(this);
    }

    @Override
    protected void onStart() {
        super.onStart();
        retrieveDeviceSerialNumber();
    }

    private void retrieveDeviceSerialNumber() {
        new Thread(() -> {
            UsbCdcConnection connection = new UsbCdcConnection();
            if (!connection.FindSupportedDevice(getApplicationContext())) {
                throw new RuntimeException("Supported USB device not found");
            }
            if (!connection.OpenConnection()) {
                throw new RuntimeException("Couldn't open connection to USB device");
            }
            com.intel.realsenseid.api.AndroidSerialConfig config = new com.intel.realsenseid.api.AndroidSerialConfig();
            config.setFileDescriptor(connection.GetFileDescriptor());
            config.setReadEndpoint(connection.GetReadEndpointAddress());
            config.setWriteEndpoint(connection.GetWriteEndpointAddress());
            DeviceController dc = new DeviceController();
            dc.Connect(config);
            String[] sn = new String[] {""};
            Status s = dc.QuerySerialNumber(sn);
            if (s == Status.Ok) {
                deviceSerialNumber = sn[0];
            } else {
                deviceSerialNumber = null;
            }
            dc.Disconnect();
        }).start();
    }

    @Override
    public void onClick(@NotNull FileModel fileModel) {
        if (fileModel.getFileType() == FileType.FOLDER) {
            addFileFragment(fileModel);
        } else {
            handleFileSelection(fileModel);
        }
    }

    @Override
    public void onLongClick(FileModel fileModel) {

    }

    private void handleFileSelection(@NotNull FileModel fileModel) {
        String path = fileModel.getPath();
        FwUpdater fwu = new FwUpdater();
        String[] fwVersion = new String[]{""};
        String[] recognitionVersion = new String[]{""};
        StringVector moduleNames = new StringVector();
        boolean versionExtractionStatus = fwu.ExtractFwInformation(path, fwVersion, recognitionVersion, moduleNames);
        String firmwareVersion = fwVersion[0];
        if (!versionExtractionStatus) {
            showIncompatibleFileDialog(fileModel.getName());
            return;
        }
        if (!fwu.IsEncryptionSupported(fileModel.getPath(), deviceSerialNumber)) {
            showIncompatibleSkuDialog(deviceSerialNumber);
            return;
        }
        if (!SwigWrapper.IsFwCompatibleWithHost(firmwareVersion)) {
            showIncompatibleVersionDialog(fileModel, firmwareVersion, recognitionVersion[0]);
            return;
        }
        checkPolicyAndProceed(fileModel, firmwareVersion, recognitionVersion[0]);
    }

    private void checkPolicyAndProceed(@NotNull FileModel fileModel, String firmwareVersion, String recognitionVersion) {
        FwUpdater.UpdatePolicyInfo policyInfo =  firmwareUpdateLogic.decideUpdatePolicy(fileModel.getPath());
        if (policyInfo.getPolicy() == FwUpdater.UpdatePolicyInfo.UpdatePolicy.NOT_ALLOWED)
        {
            showPolicyNotAllowedDialog();
            return;
        }
        if  (policyInfo.getPolicy() == FwUpdater.UpdatePolicyInfo.UpdatePolicy.REQUIRE_INTERMEDIATE_FW)
        {
            showPolicyRequireIntermediateDialog(policyInfo.getIntermediate());
            return;
        }
        showUpdateConfirmationDialog(fileModel, firmwareVersion, recognitionVersion);
    }

    private void showPolicyRequireIntermediateDialog(String intermediateVersion) {
        new AlertDialog.Builder(this)
                .setTitle("Incompatible firmware version")
                .setMessage("Firmware cannot be updated directly to the chosen version.\n" +
                        "Flash firmware version " + intermediateVersion + " first.\n")
                .setPositiveButton(android.R.string.ok, null).show();
    }

    private void showPolicyNotAllowedDialog() {
        new AlertDialog.Builder(this)
                .setTitle("Incompatible firmware version")
                .setMessage("Update from current device firmware to selected firmware file\n" +
                        "is unsupported by this host application.\n")
                .setPositiveButton(android.R.string.ok, null).show();
    }

    private void showIncompatibleSkuDialog(String deviceSerialNumber) {
        String sku = DeviceSerialNumberToSku(deviceSerialNumber);
        new AlertDialog.Builder(this)
                .setTitle("Incompatible firmware SKU")
                .setMessage("Selected firmware version is incompatible with your F450 model.\n" +
                        "Please make sure the firmware file you're using is for " + sku +
                        " devices and try again.\n")
                .setPositiveButton(android.R.string.ok, null).show();
    }

    private String DeviceSerialNumberToSku(String deviceSerialNumber)
    {
        Pattern sku2Option1 = Pattern.compile("12[02]\\d6228\\d{4}.*");
        Pattern sku2Option2 = Pattern.compile("\\d{4}6229\\d{4}.*");

        if (sku2Option1.matcher(deviceSerialNumber).matches() || sku2Option2.matcher(deviceSerialNumber).matches())
        {
            return "sku2";
        }
        return "sku1";
    }

    private void showIncompatibleVersionDialog(@NotNull FileModel fileModel, String firmwareVersion, String recognitionVersion) {
        new AlertDialog.Builder(this)
                .setTitle("Incompatible firmware version")
                .setMessage(fileModel.getName() + " is of firmware version " + firmwareVersion +
                        "\nwhich is INCOMPATIBLE with the host version\n" +
                        "Host version: " + SwigWrapper.Version() + "\n" +
                        "Compatible firmware versions: " + SwigWrapper.CompatibleFirmwareVersion()
                        + "\nProceed anyway?")
                .setPositiveButton(android.R.string.yes, ((dialogInterface, i) -> {
                    checkPolicyAndProceed(fileModel, firmwareVersion, recognitionVersion);
                }))
                .setNegativeButton(android.R.string.no, null)
                .show();
    }

    private void showIncompatibleFileDialog(String filename) {
        new AlertDialog.Builder(this)
                .setTitle("Unsupported file")
                .setMessage("Could not extract firmware version from file. " + filename + " is " +
                        "not a valid firmware file.")
                .setPositiveButton(android.R.string.ok, null).show();
    }

    private void showUpdateConfirmationDialog(@NotNull FileModel fileModel, String firmwareVersion,
                                              String recognitionVersion) {
        String stringBuilder = "Are you sure you want to flash the following firmware?\n" +
                "Filename: " + fileModel.getName() + "\n" +
                "Firmware version: " + firmwareVersion + "\n" +
                "Recognition version: " + recognitionVersion;
        new AlertDialog.Builder(this)
                .setTitle("Flash new firmware")
                .setMessage(stringBuilder)
                .setPositiveButton(android.R.string.ok, (dialogInterface, i) -> {
                    switchToProgressFragment();
                    firmwareUpdateLogic.updateFirmware(fileModel.getPath());
                })
                .setNegativeButton(android.R.string.cancel, null).show();
    }

    private void switchToProgressFragment() {
        UpdateProgressFragment updateProgressFragment = new UpdateProgressFragment();
        getSupportFragmentManager().beginTransaction()
                .replace(R.id.container, updateProgressFragment)
                .commit();
    }

    private void addFileFragment(@NotNull FileModel fileModel) {
        FilesListFragment filesListFragment = new FilesListFragment(fileModel.getPath());
        getSupportFragmentManager().beginTransaction()
                .replace(R.id.container, filesListFragment)
                .addToBackStack(fileModel.getPath())
                .commit();
    }

    @Override
    public void onBackPressed() {
        super.onBackPressed();
        if (getSupportFragmentManager().getBackStackEntryCount() == 0) {
            finish();
        }
    }
}