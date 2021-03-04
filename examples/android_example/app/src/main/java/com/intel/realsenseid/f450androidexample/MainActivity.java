package com.intel.realsenseid.f450androidexample;

import android.app.Activity;
import android.content.Context;
import android.os.AsyncTask;
import android.os.Bundle;
import android.text.Layout;
import android.text.method.ScrollingMovementMethod;
import android.util.Log;
import android.view.Menu;
import android.view.MenuItem;
import android.view.TextureView;
import android.view.View;
import android.widget.AdapterView;
import android.widget.ArrayAdapter;
import android.widget.EditText;
import android.widget.ListView;
import android.widget.TextView;
import android.widget.Toast;

import com.intel.realsenseid.api.AuthConfig;
import com.intel.realsenseid.api.AuthenticateStatus;
import com.intel.realsenseid.api.AuthenticationCallback;
import com.intel.realsenseid.api.EnrollStatus;
import com.intel.realsenseid.api.EnrollmentCallback;
import com.intel.realsenseid.api.FaceAuthenticator;
import com.intel.realsenseid.api.FacePose;
import com.intel.realsenseid.api.Preview;
import com.intel.realsenseid.api.PreviewConfig;
import com.intel.realsenseid.api.Status;
import com.intel.realsenseid.impl.UsbCdcConnection;

import java.util.ArrayList;
import java.util.List;

import androidx.appcompat.app.AppCompatActivity;
import androidx.appcompat.widget.Toolbar;


public class MainActivity extends AppCompatActivity {
    private static final String TAG = "AndroidF450Sample";
    public static final int ID_MAX_LENGTH = 15;
    private UsbCdcConnection m_UsbCdcConnection;
    FaceAuthenticator m_faceAuthenticator;
    Preview m_preview;
    AndroidPreviewImageReadyCallback m_previewCallback;

    private TextView m_messagesTV;
    private List<Integer> m_buttonIds;
    private boolean m_flipOrientation;
    private boolean m_allowMasks;
    private ArrayList<String> m_userIds;
    private String m_selectedId;
    private TextureView m_previewTxv;
    private static final int MAX_NUMBER_OF_USERS_TO_SHOW = 25;
    private Menu m_optionsMenu;
    private FaceAuthenticatorCreator m_faceAuthenticatorCreator;


    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
        Toolbar toolbar = findViewById(R.id.toolbar);
        setSupportActionBar(toolbar);
        Log.d(TAG, "onCreate");
        ShowMainUI();
        m_messagesTV = (TextView) findViewById(R.id.txt_messages);
        m_messagesTV.setMovementMethod(new ScrollingMovementMethod());
        m_buttonIds = new ArrayList<>();
        m_buttonIds.add(R.id.btn_authenticate);
        m_buttonIds.add(R.id.btn_enroll);
        m_buttonIds.add(R.id.btn_remove);
        m_buttonIds.add(R.id.btn_remove_all);
        m_optionsMenu = null;
        m_flipOrientation = false;
        m_allowMasks = false;
        m_userIds = new ArrayList<>();
        m_selectedId = null;
        m_previewTxv = (TextureView) findViewById(R.id.txv_preview);
        System.loadLibrary("RealSenseIDSwigJNI");
    }

    @Override
    public boolean onCreateOptionsMenu(Menu menu) {
        super.onCreateOptionsMenu(menu);
        m_optionsMenu = menu;
        // Inflate the menu; this adds items to the action bar if it is present.
        getMenuInflater().inflate(R.menu.menu_main, menu);
        menu.findItem(R.id.action_flip_camera).setChecked(m_flipOrientation);
        menu.findItem(R.id.action_allow_mask).setChecked(m_allowMasks);
        return true;
    }

    private void setAuthSettingsToUI() {
        if (null == m_optionsMenu) {
            // Menu not created yet. Items will get the correct values when inflated
            // We are done here
            return;
        }
        runOnUiThread(new Runnable() {
            @Override
            public void run() {
                m_optionsMenu.findItem(R.id.action_flip_camera).setChecked(m_flipOrientation);
                m_optionsMenu.findItem(R.id.action_allow_mask).setChecked(m_allowMasks);
            }
        });
    }

    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
        int id = item.getItemId();
        item.setChecked(!item.isChecked());
        switch (id) {
            case R.id.action_flip_camera:
                m_flipOrientation = item.isChecked();
                ApplyAuthenticationSettings();
                m_previewCallback.SetOrientation(m_flipOrientation);
                break;
            case R.id.action_allow_mask:
                m_allowMasks = item.isChecked();
                ApplyAuthenticationSettings();
                break;
            case R.id.action_standby:
                new Thread(new Runnable() {
                    public void run() {
                        DoWhileConnected(() -> {
                            setEnableToList(false);
                            Status s = m_faceAuthenticator.Standby();
                            Log.d(TAG, String.format("Standby action status: %s", s.toString()));
                            setEnableToList(true);
                        });
                    }
                }).start();
                break;
            default:
                /**
                 * Very un-elegant, but this is a way to make the code that calls Pair and Unpair
                 * and is supported only in "secured" mode exist in secure mode, and not break
                 * the compilation of "unsecured" mode, without keeping a clone of
                 * "MainActivity.java" for each build flavor.
                 *
                 *  TODO: Consider re-designing this
                 */
                new Thread(new Runnable() {
                    public void run() {
                        DoWhileConnected(() -> {
                            setEnableToList(false);
                            MenuHelper.HandleSelection(id, m_faceAuthenticator);
                            setEnableToList(true);
                        });
                    }
                }).start();
        }
        return true;
    }

    private void ApplyAuthenticationSettings() {
        new Thread(new Runnable() {
            public void run() {
                DoWhileConnected(() -> {
                    setEnableToList(false);
                    Status s = m_faceAuthenticator.SetAuthSettings(getUIAuthConfig());
                    Log.d(TAG, String.format("Setting authentication settings status: %s", s.toString()));
                    setEnableToList(true);
                });
            }
        }).start();
    }

    private AuthConfig getUIAuthConfig() {
        AuthConfig ac = new AuthConfig();
        ac.setCamera_rotation(m_flipOrientation ? AuthConfig.CameraRotation.Rotation_180_Deg : AuthConfig.CameraRotation.Rotation_0_Deg);
        ac.setSecurity_level(m_allowMasks ? AuthConfig.SecurityLevel.Medium : AuthConfig.SecurityLevel.High);
        return ac;
    }

    @Override
    protected void onStart() {
        Log.d(TAG, "onStart");
        super.onStart();
        buildFaceBiometricsOnThread();
    }

    @Override
    protected void onStop() {
        Log.d(TAG, "onStop");
        DeconstructFaceAuthenticator();
        super.onStop();
    }

    private  void buildPreview(){
        final Activity activity = this;
        PreviewConfig previewConfig= new PreviewConfig();
        AppendToTextView(activity,"Preview starting...");
        previewConfig.setCameraNumber(m_UsbCdcConnection.GetFileDescriptor());
        m_preview = new Preview(previewConfig);
        m_previewCallback = new AndroidPreviewImageReadyCallback(m_previewTxv, m_flipOrientation);
        m_preview.StartPreview(m_previewCallback);
        AppendToTextView(activity,"Preview started");
    }

    private void buildFaceBiometricsOnThread() {
        final Activity activity = this;
        AsyncTask.execute(new Runnable() {
            @Override
            public void run() {
                Log.d(TAG, "AsyncTask: buildFaceBiometricsOnThread");
                m_UsbCdcConnection = new UsbCdcConnection();
                if (m_UsbCdcConnection != null) {
                    Context applicationContext = MainActivity.this.getApplicationContext();
                    Log.d(TAG, "The class UsbCdcConnection was created");
                    if (!m_UsbCdcConnection.FindSupportedDevice(applicationContext)) {
                        AppendToTextView(activity, getResources().getString(R.string.device_not_found));
                        Log.e(TAG, "Supported USB device not found");
                        DeconstructFaceAuthenticator();
                        return;
                    }
                    m_UsbCdcConnection.RequestDevicePermission(applicationContext, new UsbCdcConnection.PermissionCallback() {

                        @Override
                        public void Response(boolean permissionGranted) {
                            if (permissionGranted && m_UsbCdcConnection.OpenConnection()) {
                                m_faceAuthenticatorCreator = new FaceAuthenticatorCreator();
                                m_faceAuthenticator = m_faceAuthenticatorCreator.Create(m_UsbCdcConnection);
                                if (null != m_faceAuthenticator) {
                                    Log.d(TAG, "FaceAuthenticator class was created");
                                    DoWhileConnected(() -> {
                                        updateUIAuthSettingsFromDevice();
                                    });
                                    setEnableToList(true);
                                    buildPreview();
                                    return;
                                }
                            } else {
                                AppendToTextView(activity, getResources().getString(R.string.permission_not_granted));
                            }
                            Log.e(TAG, "Error creating the class FaceAuthenticator");
                            DeconstructFaceAuthenticator();
                        }
                    });
                }
            }
        });
    }

    private void updateUIAuthSettingsFromDevice() {
        AuthConfig ac = new AuthConfig();
        m_faceAuthenticator.QueryAuthSettings(ac);
        m_flipOrientation = ac.getCamera_rotation() == AuthConfig.CameraRotation.Rotation_180_Deg;
        m_allowMasks = ac.getSecurity_level() == AuthConfig.SecurityLevel.Medium;
        setAuthSettingsToUI();
    }

    private void setEnableToList(boolean b) {
        runOnUiThread(new Runnable() {
            @Override
            public void run() {
                for (int button : m_buttonIds) {
                    findViewById(button).setEnabled(b);
                }
            }
        });
    }

    private void DeconstructFaceAuthenticator() {
        if (m_preview != null) {
            m_preview.StopPreview();
        }

        if (m_faceAuthenticator != null) {
            m_faceAuthenticator.Disconnect();
            m_faceAuthenticator = null;
        }

        if (m_UsbCdcConnection != null) {
            m_UsbCdcConnection.CloseConnection();
            m_UsbCdcConnection = null;
        }
    }

    @Override
    protected void onResume() {
        super.onResume();
        Log.d(TAG, "onResume");
    }

    @Override
    protected void onPause() {
        Log.d(TAG, "onPause");
        super.onPause();
    }

    public void ExecuteEnrollment(View view) {
        ShowEnrollmentUI();
    }

    public void AppendToTextView(final Activity activity, final String msg) {
        final TextView tv = m_messagesTV;
        activity.runOnUiThread(new Runnable() {
            public void run() {
                tv.append(msg + "\n");
                final Layout layout = tv.getLayout();
                if (layout != null) {
                    int delta = layout.getLineBottom(tv.getLineCount() - 1) - tv.getScrollY()
                            - tv.getHeight();
                    if (delta > 0)
                        tv.scrollBy(0, delta);
                }
            }
        });
    }

    public void ExecuteEnrollOk(View view) {
        Log.d(TAG, "ExecuteEnrollment");
        if (m_faceAuthenticator == null) {
            Log.d(TAG, "faceAuthenticator is null");
            return;
        }
        setEnableToList(false);
        final Activity activity = this;
        new Thread(new Runnable() {
            public void run() {
                EditText userIdText = (EditText) findViewById(R.id.txt_user_id);
                String UserId = userIdText.getText().toString();

                EnrollmentCallback enrollmentCallback = new EnrollmentCallback() {
                    public void OnResult(EnrollStatus status) {
                        if (status == EnrollStatus.Success) {
                            Log.d(TAG, "Enrollment completed successfully");
                            AppendToTextView(activity, getResources().getString(R.string.enroll_success));
                        } else {
                            Log.d(TAG, String.format("Enrollment failed with status %s", status.toString()));
                            AppendToTextView(activity, getResources().getString(R.string.enroll_fail));
                        }
                    }

                    public void OnProgress(FacePose pose) {
                        Log.d(TAG, String.format("Enrollment progress. pose: %s", pose.toString()));
                    }

                    public void OnHint(EnrollStatus hint) {
                        Log.d(TAG, String.format("Enrollment hint: %s", hint.toString()));
                        if (hint != EnrollStatus.CameraStarted && hint != EnrollStatus.CameraStopped && hint != EnrollStatus.FaceDetected && hint != EnrollStatus.LedFlowSuccess)
                            AppendToTextView(activity, hint.toString());
                    }
                };
                DoWhileConnected(() -> {
                    Status enrollStatus = m_faceAuthenticator.Enroll(enrollmentCallback, UserId);
                    Log.d(TAG, "Enrollment done with status: " + enrollStatus.toString());
                });
                setEnableToList(true);

            }
        }).start();
        ShowMainUI();
    }

    void DoWhileConnected(ConnectionWrappedFunction f) {
        m_faceAuthenticator.Connect(
                m_UsbCdcConnection.GetFileDescriptor(),
                m_UsbCdcConnection.GetReadEndpointAddress(),
                m_UsbCdcConnection.GetWriteEndpointAddress());
        f.Do();
        m_faceAuthenticator.Disconnect();
    }


    public void ExecuteEnrollCancel(View view) {
        ShowMainUI();
    }

    private void ShowEnrollmentUI() {
        findViewById(R.id.btn_authenticate).setVisibility(View.GONE);
        findViewById(R.id.btn_enroll).setVisibility(View.GONE);
        findViewById(R.id.btn_remove).setVisibility(View.GONE);
        EditText et = (EditText) findViewById(R.id.txt_user_id);
        et.setText("");
        et.setVisibility(View.VISIBLE);
        findViewById(R.id.btn_enroll_ok).setVisibility(View.VISIBLE);
        findViewById(R.id.btn_enroll_cancel).setVisibility(View.VISIBLE);
    }

    private void ShowMainUI() {
        findViewById(R.id.txv_preview).setVisibility(View.VISIBLE);
        findViewById(R.id.btn_authenticate).setVisibility(View.VISIBLE);
        findViewById(R.id.btn_enroll).setVisibility(View.VISIBLE);
        findViewById(R.id.btn_remove).setVisibility(View.VISIBLE);
        findViewById(R.id.txt_user_id).setVisibility(View.GONE);
        findViewById(R.id.btn_enroll_ok).setVisibility(View.GONE);
        findViewById(R.id.btn_enroll_cancel).setVisibility(View.GONE);
        findViewById(R.id.lst_enrolled_ids).setVisibility(View.GONE);
        findViewById(R.id.btn_remove_selected).setVisibility(View.GONE);
        findViewById(R.id.btn_remove_all).setVisibility(View.GONE);
        findViewById(R.id.btn_back_from_remove).setVisibility(View.GONE);
    }

    private void ShowRemoveUI() {
        findViewById(R.id.txv_preview).setVisibility(View.GONE);
        findViewById(R.id.btn_authenticate).setVisibility(View.GONE);
        findViewById(R.id.btn_enroll).setVisibility(View.GONE);
        findViewById(R.id.btn_remove).setVisibility(View.GONE);
        findViewById(R.id.lst_enrolled_ids).setVisibility(View.VISIBLE);
        findViewById(R.id.btn_remove_selected).setVisibility(View.VISIBLE);
        findViewById(R.id.btn_remove_all).setVisibility(View.VISIBLE);
        findViewById(R.id.btn_back_from_remove).setVisibility(View.VISIBLE);
    }

    public void ExecuteAuthentication(View view) {
        Log.d(TAG, "ExecuteAuthentication");
        if (m_faceAuthenticator == null) {
            Log.d(TAG, "faceAuthenticator is null");
            return;
        }
        setEnableToList(false);
        final Activity activity = this;
        new Thread(new Runnable() {
            @Override
            public void run() {
                // a potentially time consuming task
                AuthenticationCallback authenticationCallback = new AuthenticationCallback() {
                    public void OnResult(AuthenticateStatus status, String userId) {
                        if (status == AuthenticateStatus.Success) {
                            Log.d(TAG, String.format("Authentication allowed. UserId %s", userId));
                            AppendToTextView(activity, getResources().getString(R.string.authenticate_greet) + " " + userId);
                        } else {
                            Log.d(TAG, "Authentication forbidden");
                            AppendToTextView(activity, getResources().getString(R.string.authenticate_forbidden));
                        }
                    }

                    public void OnHint(AuthenticateStatus hint) {
                        Log.d(TAG, String.format("Authentication hint: %s", hint.toString()));
                        if (hint != AuthenticateStatus.CameraStarted && hint != AuthenticateStatus.CameraStopped && hint != AuthenticateStatus.FaceDetected && hint != AuthenticateStatus.LedFlowSuccess)
                            AppendToTextView(activity, hint.toString());
                    }
                };
                DoWhileConnected(() -> {
                    Status authenticateStatus = m_faceAuthenticator.Authenticate(authenticationCallback);
                    Log.d(TAG, "Authentication done with status: " + authenticateStatus.toString());
                });
                setEnableToList(true);
            }
        }).start();
    }

    public void ExecuteShowRemoveUI(View view) {
        Log.d(TAG, "ExecuteShowRemoveUI");
        if (m_faceAuthenticator == null) {
            Log.d(TAG, "faceAuthenticator is null");
            return;
        }
        m_userIds.clear();
        findViewById(R.id.btn_remove_selected).setEnabled(false);
        setEnableToList(false);
        ShowRemoveUI();
        new Thread(new Runnable() {
            @Override
            public void run() {
                DoWhileConnected(() -> {
                    FetchUserIds();
                    UpdateUsersList();
                    setEnableToList(true);
                });
            }
        }).start();
    }

    private AdapterView.OnItemClickListener userIdClickedHandler = new AdapterView.OnItemClickListener() {
        public void onItemClick(AdapterView parent, View v, int position, long id) {
            m_selectedId = m_userIds.get(position);
            findViewById(R.id.btn_remove_selected).setEnabled(true);
        }
    };

    private void UpdateUsersList() {
        ArrayAdapter<String> adapter = new ArrayAdapter<String>(this,
                android.R.layout.simple_list_item_1, m_userIds);
        runOnUiThread(new Runnable() {
            @Override
            public void run() {
                ListView listView = (ListView) findViewById(R.id.lst_enrolled_ids);
                listView.setAdapter(adapter);
                listView.setOnItemClickListener(userIdClickedHandler);
            }
        });

    }

    public void ExecuteRemoveAll(View view) {
        Log.d(TAG, "ExecuteRemoveAll");
        if (m_faceAuthenticator == null) {
            Log.d(TAG, "faceAuthenticator is null");
            return;
        }

        setEnableToList(false);
        final Activity activity = this;
        new Thread(new Runnable() {
            @Override
            public void run() {
                DoWhileConnected(() -> {
                    Status res = m_faceAuthenticator.RemoveAll();
                    if (res.equals(Status.Ok)) {
                        AppendToTextView(activity, getResources().getString(R.string.remove_all_success));
                        m_userIds.clear();
                    }
                    else {
                        AppendToTextView(activity, getResources().getString(R.string.remove_all_fail));
                    }
                    Log.d(TAG, "RemoveAll done with res: " + res);
                    FetchUserIds();
                });
                setEnableToList(true);
                UpdateUsersList();
            }
        }).start();
    }

    public void ExecuteClearMessages(View view) {
        m_messagesTV.setText("");
        m_messagesTV.scrollTo(0, 0);
    }

    public void ExecuteBackFromRemove(View view) {
        ShowMainUI();
    }

    private void NotifyAboutDeviceQuery() {
        final Activity activity = this;
        runOnUiThread(new Runnable() {
            @Override
            public void run() {
                Toast.makeText(activity, "Querying device for user IDs...", Toast.LENGTH_SHORT).show();
            }
        });
    }

    private void FetchUserIds() {
        NotifyAboutDeviceQuery();
        String[] ids = new String[MAX_NUMBER_OF_USERS_TO_SHOW];
        for (int i = 0 ; i < ids.length ; ++i) {
            ids[i] = new String(new char[ID_MAX_LENGTH]); // string of ID_MAX_LENGTH characters as a placeholder. not including \0 at the end that is required in char[]
        }
        long[] numOfIds = new long[]{MAX_NUMBER_OF_USERS_TO_SHOW};
        Status s = m_faceAuthenticator.QueryUserIds(ids, numOfIds);
        m_userIds.clear();
        if (s == Status.Ok) {
            for (int i = 0 ; i<numOfIds[0] ; ++i) {
                m_userIds.add(ids[i]);
            }
        }
    }

    public void RemoveSelected(View view) {
        setEnableToList(false);
        findViewById(R.id.btn_remove_selected).setEnabled(false);
        final Activity activity = this;
        new Thread(new Runnable() {
            @Override
            public void run() {
                DoWhileConnected(() -> {
                    Status res = m_faceAuthenticator.RemoveUser(m_selectedId);
                    if (res.equals(Status.Ok)) {
                        AppendToTextView(activity, "Removed user: " + m_selectedId);
                        m_userIds.clear();
                    }
                    else {
                        AppendToTextView(activity, "Failed to remove user");
                    }
                    Log.d(TAG, "Remove selected done with res: " + res);
                    FetchUserIds();
                });
                setEnableToList(true);
                UpdateUsersList();
            }
        }).start();
    }
}