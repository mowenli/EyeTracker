package com.example.stellasong.eyetracker;

import android.graphics.Bitmap;

public class EyetrackerNcnn {
    public native boolean Init(byte[] param, byte[] bin);

    public native String Detect(Bitmap image_left, Bitmap image_right, Bitmap image_face, byte[] face_grid);

    static {
        System.loadLibrary("eyetrackerncnn");
    }
}
