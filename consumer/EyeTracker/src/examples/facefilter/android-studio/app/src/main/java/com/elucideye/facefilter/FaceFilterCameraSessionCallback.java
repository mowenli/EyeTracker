package com.elucideye.facefilter;

import android.hardware.camera2.CameraAccessException;
import android.hardware.camera2.CameraCaptureSession;
import android.hardware.camera2.CameraDevice;
import android.hardware.camera2.CaptureRequest;
import android.os.Handler;
import android.util.Log;
import android.view.Surface;

// All methods except constructor called from "camera thread"
public class FaceFilterCameraSessionCallback extends CameraCaptureSession.StateCallback {
    static private String TAG = "ElucideyeFaceFilterSessionCallback";

    private CaptureRequest.Builder mPreviewRequestBuilder;
    private Handler mBackgroundHandler;
    private FaceFilterCameraManager mCameraManager;

    FaceFilterCameraSessionCallback(FaceFilterCameraManager cameraManager, CameraDevice cameraDevice,
                                    Handler backgroundHandler, Surface cameraOutputSurface) throws CameraAccessException {
        mCameraManager = cameraManager;
        mBackgroundHandler = backgroundHandler;
        mPreviewRequestBuilder = cameraDevice.createCaptureRequest(CameraDevice.TEMPLATE_PREVIEW);
        mPreviewRequestBuilder.addTarget(cameraOutputSurface);
    }

    @Override
    public void onConfigured(CameraCaptureSession cameraCaptureSession) {
        Log.d(TAG, "onConfigured");

        mCameraManager.setCaptureSession(cameraCaptureSession);
        try {
            mPreviewRequestBuilder.set(CaptureRequest.CONTROL_AF_MODE, CaptureRequest.CONTROL_AF_MODE_CONTINUOUS_PICTURE);
            mPreviewRequestBuilder.set(CaptureRequest.CONTROL_AE_MODE, CaptureRequest.CONTROL_AE_MODE_ON_AUTO_FLASH);

            cameraCaptureSession.setRepeatingRequest(mPreviewRequestBuilder.build(), null, mBackgroundHandler);
        } catch (CameraAccessException e) {
            Log.e(TAG, "createCaptureSession");
        }
    }
}
