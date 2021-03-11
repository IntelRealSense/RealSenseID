package com.intel.realsenseid.f450androidexample;

import android.annotation.SuppressLint;
import android.content.DialogInterface;
import android.os.Bundle;
import android.os.Environment;
import android.widget.ProgressBar;

import com.intel.realsenseid.api.AndroidSerialConfig;
import com.intel.realsenseid.api.FwUpdater;
import com.intel.realsenseid.api.SwigWrapper;
import com.intel.realsenseid.impl.UsbCdcConnection;

import org.jetbrains.annotations.NotNull;

import androidx.appcompat.app.AlertDialog;
import androidx.appcompat.app.AppCompatActivity;

public class FilesListActivity extends AppCompatActivity implements FilesListFragment.OnViewItemClickListener {

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
    }

    @Override
    public void onClick(@NotNull FileModel fileModel) {
        if (fileModel.getFileType() == FileType.FOLDER) {
            addFileFragment(fileModel);
        } else {
            handleFileSelection(fileModel);
        }
    }

    private void handleFileSelection(@NotNull FileModel fileModel) {
        String path = fileModel.getPath();
        FwUpdater fwu = new FwUpdater();
        String[] fwVersion = new String[]{""};
        boolean versionExtractionStatus = fwu.ExtractFwVersion(path, fwVersion);
        String firmwareVersion = fwVersion[0];
        if (!versionExtractionStatus) {
            showIncompatibleFileDialog(fileModel.getName());
            return;
        }
        // Something seems to be wrong wit the version compatibility check. Disabled for now.
        if (!SwigWrapper.IsFwCompatibleWithHost(firmwareVersion)) {
            showIncompatibleVersionDialog(fileModel.getName(), firmwareVersion);
            return;
        }
        showUpdateConfirmationDialog(fileModel, firmwareVersion);
    }

    private void showIncompatibleVersionDialog(String fileName, String firmwareVersion) {
        new AlertDialog.Builder(this)
                .setTitle("Incompatible firmware version")
                .setMessage(fileName + " is of firmware version " + firmwareVersion + " which is " +
                        "incompatible with host version\n" +
                        "Host version: " + SwigWrapper.Version() + "\n" +
                        "Compatible firmware versions: " + SwigWrapper.CompatibleFirmwareVersion())
                .setPositiveButton(android.R.string.ok, null).show();
    }

    private void showIncompatibleFileDialog(String filename) {
        new AlertDialog.Builder(this)
                .setTitle("Unsupported file")
                .setMessage("Could not extract firmware version from file. " + filename + " is " +
                        "not a valid firmware file.")
                .setPositiveButton(android.R.string.ok, null).show();
    }

    private void showUpdateConfirmationDialog(@NotNull FileModel fileModel, String firmwareVersion) {
        StringBuilder stringBuilder = new StringBuilder();
        stringBuilder.append("Are you sure you want to flash the following firmware?\n")
        .append("Filename: ").append(fileModel.getName()).append("\n")
        .append("Firmware version: ").append(firmwareVersion);
        new AlertDialog.Builder(this)
                .setTitle("Flash new firmware")
                .setMessage(stringBuilder.toString())
                .setPositiveButton(android.R.string.ok, new DialogInterface.OnClickListener() {
                    @Override
                    public void onClick(DialogInterface dialogInterface, int i) {
                        switchToProgressFragment();
                        updateFirmware(fileModel);
                    }
                })
                .setNegativeButton(android.R.string.cancel, null).show();
    }

    private void updateFirmware(@NotNull FileModel fileModel) {
        new Thread(new Runnable() {
            public void run() {
                String path = fileModel.getPath();
                FwUpdater fwu = new FwUpdater();
                UsbCdcConnection connection = new UsbCdcConnection();
                if (connection == null) {
                    throw new RuntimeException("Error opening a USB connection");
                }
                if (!connection.FindSupportedDevice(getApplicationContext())) {
                    throw new RuntimeException("Supported USB device not found");
                }
                if (!connection.OpenConnection()) {
                    throw new RuntimeException("Couldn't open connection to USB device");
                }

                AndroidSerialConfig config = new AndroidSerialConfig();
                config.setFileDescriptor(connection.GetFileDescriptor());
                config.setReadEndpoint(connection.GetReadEndpointAddress());
                config.setWriteEndpoint(connection.GetWriteEndpointAddress());
                FwUpdater.Settings settings = new FwUpdater.Settings();
                settings.setAndroid_config(config);
                FwUpdater.EventHandler handler = new FwUpdater.EventHandler() {
                    @SuppressLint("DefaultLocale")
                    @Override
                    public void OnProgress(float progress) {
                        runOnUiThread(new Runnable() {
                            @Override
                            public void run() {
                                ProgressBar pb = (ProgressBar) findViewById(R.id.update_progress_progressBar);
                                pb.setProgress((int) (progress * 100));
                            }
                        });
                    }
                };
                fwu.Update(handler, settings, path);
                connection.CloseConnection();
                runOnUiThread(new Runnable() {
                    @Override
                    public void run() {
                        finish();
                    }
                });
            }
        }).start();
    }


    @Override
    public void onLongClick(FileModel fileModel) {

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