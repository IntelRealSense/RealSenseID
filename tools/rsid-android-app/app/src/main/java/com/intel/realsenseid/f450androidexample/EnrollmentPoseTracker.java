package com.intel.realsenseid.f450androidexample;

import com.intel.realsenseid.api.FacePose;

import java.util.LinkedList;
import java.util.List;

public class EnrollmentPoseTracker {
    private List<TrackedPose> m_trackedPoses;

    private class TrackedPose {

        private final FacePose m_pose;
        private boolean m_check;

        public TrackedPose(FacePose p) {
            m_pose = p;
            m_check = false;
        }


        public FacePose getPose() {
            return m_pose;
        }

        public boolean isCheck() {
            return m_check;
        }

        public void setCheck() {
            this.m_check = true;
        }
    }

    public EnrollmentPoseTracker(FacePose[] facePoses) {
        m_trackedPoses = new LinkedList<>();
        for (FacePose p : facePoses) {
            m_trackedPoses.add(new TrackedPose(p));
        }
    }

    public void markPoseCheck(FacePose pose) {
        for (TrackedPose tp: m_trackedPoses) {
            if ((tp.getPose() == pose) && !tp.isCheck()) {
                tp.setCheck();
                return;
            }
        }
    }

    public FacePose getNext() {
        for (TrackedPose tp: m_trackedPoses) {
            if (!tp.isCheck()) {
                return tp.getPose();
            }
        }
        return null;
    }
}
