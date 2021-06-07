package com.intel.realsenseid.f450androidexample;

import android.content.DialogInterface;
import android.os.Bundle;
import android.os.Environment;

import com.intel.realsenseid.api.FwUpdater;
import com.intel.realsenseid.api.StringVector;
import com.intel.realsenseid.api.SwigWrapper;
import com.intel.realsenseid.api.StringVector;

import org.jetbrains.annotations.NotNull;

import androidx.appcompat.app.AlertDialog;
import androidx.appcompat.app.AppCompatActivity;

public class FilesListActivity extends AppCompatActivity implements FilesListFragment.OnViewItemClickListener {

    private FirmwareUpdateLogic firmwareUpdateLogic;

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
        String[] recognitionVersion = new String[]{""};
        StringVector moduleNames = new StringVector();
        boolean versionExtractionStatus = fwu.ExtractFwInformation(path, fwVersion, recognitionVersion, moduleNames);
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
        showUpdateConfirmationDialog(fileModel, firmwareVersion, recognitionVersion[0]);
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

    private void showUpdateConfirmationDialog(@NotNull FileModel fileModel, String firmwareVersion,
                                              String recognitionVersion) {
        StringBuilder stringBuilder = new StringBuilder();
        stringBuilder.append("Are you sure you want to flash the following firmware?\n")
        .append("Filename: ").append(fileModel.getName()).append("\n")
        .append("Firmware version: ").append(firmwareVersion).append("\n")
        .append("Recognition version: ").append(recognitionVersion);
        new AlertDialog.Builder(this)
                .setTitle("Flash new firmware")
                .setMessage(stringBuilder.toString())
                .setPositiveButton(android.R.string.ok, new DialogInterface.OnClickListener() {
                    @Override
                    public void onClick(DialogInterface dialogInterface, int i) {
                        switchToProgressFragment();
                        firmwareUpdateLogic.updateFirmware(fileModel);
                    }
                })
                .setNegativeButton(android.R.string.cancel, null).show();
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