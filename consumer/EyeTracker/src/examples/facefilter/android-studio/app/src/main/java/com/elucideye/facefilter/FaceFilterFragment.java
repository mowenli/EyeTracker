package com.elucideye.facefilter;

import android.app.Activity;
import android.graphics.Color;
import android.os.Bundle;
import android.support.v4.app.ActivityCompat;
import android.support.v4.app.Fragment;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.hardware.SensorManager;
import android.widget.ImageButton;
import android.widget.LinearLayout;
import android.widget.RelativeLayout;
import android.widget.Space;

public class FaceFilterFragment extends Fragment implements ActivityCompat.OnRequestPermissionsResultCallback {

    private static String TAG = "ElucideyeFaceFilterFragment";
    private FaceFilterCameraManager mCameraManager = null;

    /**
    * An {@link FaceFilterGLSurfaceView} for camera preview.
    */
    private FaceFilterGLSurfaceView mGLSurfaceView;

    public static FaceFilterFragment newInstance(FaceFilterCameraManager cameraManager) {
        Log.i("~~~", "FaceFilterFragment: ");
        FaceFilterFragment fragment = new FaceFilterFragment();
        fragment.mCameraManager = cameraManager;
        return fragment;
    }

    @Override
    public View onCreateView(LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState) {
        Log.i("~~~", "onCreateView: FaceFilterFragment");
        boolean attachToRoot = false;
        return inflater.inflate(R.layout.fragment, container, attachToRoot);
    }

    @Override
    public void onViewCreated(final View view, Bundle savedInstanceState) {
        mGLSurfaceView = view.findViewById(R.id.texture);
        mGLSurfaceView.onViewCreated(mCameraManager, this);
    }

    @Override
    public void onResume() {
        Log.d(TAG, "onResume");
        Log.i("~~~", "onResume: FaceFilterFragment");
        super.onResume();
        mGLSurfaceView.onResume();
        mCameraManager.onResume();
    }

    @Override
    public void onPause() {
        Log.d(TAG, "onPause");
        mCameraManager.onPause();
        mGLSurfaceView.onPause();
        super.onPause();
    }

    static { System.loadLibrary("facefilter_bindings"); }
}
