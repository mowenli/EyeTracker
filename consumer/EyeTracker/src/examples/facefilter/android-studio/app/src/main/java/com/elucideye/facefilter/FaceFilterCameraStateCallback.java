package com.elucideye.facefilter;

import android.hardware.camera2.CameraDevice;
import android.util.Log;

import java.util.concurrent.Semaphore;

// All methods except constructor called from "camera thread"
public class FaceFilterCameraStateCallback extends CameraDevice.StateCallback {

    private static String TAG = "ElucideyeFaceFilterCameraStateCallback";
    private FaceFilterCameraManager mCameraManager;

    FaceFilterCameraStateCallback(FaceFilterCameraManager cameraManager) {
        mCameraManager = cameraManager;
    }

    @Override
    public void onOpened(CameraDevice cameraDevice) {
        Log.d(TAG, "onOpened");
        mCameraManager.onCameraOpenResult(cameraDevice);
    }

    @Override
    public void onDisconnected(CameraDevice cameraDevice) {
        Log.d(TAG, "onDisconnected");
        cameraDevice.close();
        mCameraManager.onCameraOpenResult(null);
    }

    @Override
    public void onError(CameraDevice cameraDevice, int error) {
        Log.e(TAG, "onError: " + error);
        cameraDevice.close();
        mCameraManager.onCameraOpenResult(null);
    }
}
