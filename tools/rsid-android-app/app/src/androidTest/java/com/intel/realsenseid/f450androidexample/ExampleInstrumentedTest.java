package com.intel.realsenseid.f450androidexample;

import android.content.Context;
import android.util.Log;

import com.intel.realsenseid.api.AuthenticateStatus;
import com.intel.realsenseid.api.AuthenticationCallback;
import com.intel.realsenseid.api.Status;

import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;

import androidx.test.ext.junit.rules.ActivityScenarioRule;
import androidx.test.ext.junit.runners.AndroidJUnit4;
import androidx.test.platform.app.InstrumentationRegistry;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertTrue;

/**
 * Instrumented test, which will execute on an Android device.
 *
 * @see <a href="http://d.android.com/tools/testing">Testing documentation</a>
 */
@RunWith(AndroidJUnit4.class)
public class ExampleInstrumentedTest {
    static final String TAG = "ExampleInstrumentedTest";
    @Rule
    public ActivityScenarioRule<MainActivity> activityScenarioRule
            = new ActivityScenarioRule<>(MainActivity.class);

    @Test
    public void useAppContext() {
        // Context of the app under test.
        Context appContext = InstrumentationRegistry.getInstrumentation().getTargetContext();

        assertEquals("com.intel.realsenseid.f450androidexample", appContext.getPackageName());
    }

    @Test
    public void removeAllStressTest() {
        activityScenarioRule.getScenario().onActivity(activity -> {
            for (int i = 0; i<1000 ; ++i) {
                if (0 == i%50)
                {
                    Log.d(TAG, "removeAllStressTest: in iteration " + String.valueOf(i));
                }
                final int iteration = i;
                activity.DoWhileConnected(() -> {
                    assertEquals("Connection stress test failed on iteration " + String.valueOf(iteration), Status.Ok, activity.m_faceAuthenticator.RemoveAll());
                });
            }
        });
    }

    @Test
    public void authenticateStressTest() {
        activityScenarioRule.getScenario().onActivity(activity -> {
            class BoolWrapper {
                boolean s;
            };
            final BoolWrapper k = new BoolWrapper();
            AuthenticationCallback cb = new AuthenticationCallback() {
                public void OnResult(AuthenticateStatus status, String userId) {
                    k.s = true;
                }
                public void OnHint(AuthenticateStatus hint) {
                }
            };
            for (int i = 0; i<500 ; ++i) {
                if (0 == i%10)
                {
                    Log.d(TAG, "authenticateStressTest: in iteration " + String.valueOf(i));
                }
                k.s = false;
                final int iteration = i;
                activity.DoWhileConnected(() -> {
                    Status authenticateStatus = activity.m_faceAuthenticator.Authenticate(cb);
                    assertTrue("Authentication stress test failed on iteration "
                            + String.valueOf(iteration)
                            + " latest authentication status: "
                            + authenticateStatus.toString(), k.s);
                });

            }
        });
    }
}
