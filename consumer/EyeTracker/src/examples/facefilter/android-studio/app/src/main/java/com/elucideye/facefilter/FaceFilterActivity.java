package com.elucideye.facefilter;

import android.os.Bundle;
import android.support.annotation.NonNull;
import android.support.v4.app.ActivityCompat;
import android.support.v7.app.AppCompatActivity;
import android.util.Log;

// Activity will work with 3 threads:
// 1) UI thread: It's a main thread, all UI events here, e.g. onPause/onResume
// 2) Render thread: Created automatically by GLSurfaceView, see class FaceFilterRenderer
// 3) Camera thread: Thread to receive camera related events, see FaceFilterCamera*Callback classes
public class FaceFilterActivity extends AppCompatActivity implements ActivityCompat.OnRequestPermissionsResultCallback  {

    public FaceFilterCameraManager cameraManager;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        Log.i("~~~", "onCreate: FaceFilterActivity");
        setContentView(R.layout.camera);
        if (cameraManager == null) {
            cameraManager = new FaceFilterCameraManager(this);
        }
        cameraManager.checkPermissions();
        FaceFilterFragment fragment = FaceFilterFragment.newInstance(cameraManager);
        getSupportFragmentManager().beginTransaction().replace(R.id.container, fragment).commit();
    }

    // ActivityCompat.OnRequestPermissionsResultCallback
    @Override
    public void onRequestPermissionsResult(int requestCode, @NonNull String[] permissions, @NonNull int[] grantResults) {
        cameraManager.onRequestPermissionsResult(requestCode, permissions, grantResults);
    }
}
