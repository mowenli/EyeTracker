/*
  FaceFilterRenderer.java
  
  Implementation of the facefilter rendering routines

  Copyright 2017-2018 Elucideye, Inc. All rights reserved. [All modifications]
  
  This file is released under the 3 Clause BSD License. [All modifications]

  Reference: http://maninara.blogspot.com/2015/03/render-camera-preview-using-opengl-es.html
*/
package com.elucideye.facefilter;

import android.opengl.GLSurfaceView;
import android.graphics.SurfaceTexture;

import android.content.res.AssetManager;

import android.opengl.GLES20;
import android.graphics.Point;

import android.opengl.GLES11Ext;

import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.opengles.GL10;

import android.util.Log;
import android.util.Size;
import android.view.Surface;

import java.net.DatagramPacket;
import java.net.DatagramSocket;
import java.net.InetAddress;
import java.net.SocketException;

// We will receive GLSurfaceView.Renderer events for the underlying Surface of
// FaceFilterGLSurfaceView (GLSurfaceView) object to manage our own Surface
// and SurfaceTexture mSTexture. Original Surface will not be used directly.
public class FaceFilterRenderer implements GLSurfaceView.Renderer, SurfaceTexture.OnFrameAvailableListener {
    private static String TAG = "ElucideyeFaceFilterRenderer";

    private int[] hTex;
    private SurfaceTexture mSTexture;
    private boolean mGLInit = false;
    private boolean mUpdateST = false;
    private Size mPreviewSize;
    private int mCameraOrientation;
    private float mFocalLengthInPixels;
    private FaceFilterGLSurfaceView mView;
    private FaceFilterCameraManager mCameraManager;
    private FaceFilterFragment mFragment;

    private double x = 100, y = 100;
    private DatagramSocket socket;

    FaceFilterRenderer(FaceFilterGLSurfaceView view, FaceFilterCameraManager cameraManager, FaceFilterFragment fragment) {
        mView = view;
        mCameraManager = cameraManager;
        mPreviewSize = mCameraManager.calcPreviewSize();
        mCameraOrientation = mCameraManager.getCameraOrientation();
        mFocalLengthInPixels = mCameraManager.getFocalLengthInPixels();
        mFragment = fragment;

        setAssetManager(mFragment.getActivity().getAssets());
        loadAsset("drishti_assets", "drishti_assets.json"); // JNI

        try {
            socket = new DatagramSocket();
        } catch (SocketException e) {
            // TODO Auto-generated catch block
            e.printStackTrace();
        }
    }

    // Scheduled from FaceFilterGLSurfaceView.onPause(). Executed from "render
    // thread". Thread: "render thread"
    public void onPause() {
        Log.d(TAG, "onPause");
        mGLInit = false;
        destroy(); // JNI
        mView.jniContextDestroyed();
    }

    // GLSurfaceView.Renderer
    // Thread: "render thread"
    // Note: this method can be called without previous requestRender
    @Override
    public void onDrawFrame(GL10 unused) {
        Log.d(TAG, "onDrawFrame");
        if (!mGLInit) {
            return;
        }

        boolean startDrawing = false;

        synchronized (this) {
            if (mUpdateST) {
                mSTexture.updateTexImage();
                mUpdateST = false;
                startDrawing = true;
            }
        }

        GLES20.glClear(GLES20.GL_COLOR_BUFFER_BIT);

        if (startDrawing) {
            double[] d = drawFrame(hTex[0]); // JNI
            if (x < 100 && y < 100) {
//                Log.i("~~~", "x: " + (x - d[0])* 100 + ": y: " + (y - d[1]) * 100);
                if (d[0] - x > 0.005) {
                    Log.i("~~~", "left");
                    this.sendMouseEvent("mouse", -10, 0);
                } else if (x - d[0] > 0.005) {
                    Log.i("~~~", "right");
                    this.sendMouseEvent("mouse", 10, 0);
                }
                if (d[1] - y > 0.02) {
                    Log.i("~~~", "down");
                    this.sendMouseEvent("mouse", 0, 10);
                } else if (y - d[1] > 0.01) {
                    Log.i("~~~", "up");
                    this.sendMouseEvent("mouse", 0, -10);
                }
            }
            x = d[0];
            y = d[1];
        }
    }

    // GLSurfaceView.Renderer
    // Thread: "render thread"
    @Override
    public void onSurfaceChanged(GL10 unused, int width, int height) {
        Log.d(TAG, "onSurfaceChanged");
        surfaceChanged(width, height); // JNI
    }

    // GLSurfaceView.Renderer
    // Thread: "render thread"
    @Override
    public void onSurfaceCreated(GL10 unused, EGLConfig config) {
        Log.d(TAG, "onSurfaceCreated");
        initTex();

        mSTexture = new SurfaceTexture(hTex[0]);
        mSTexture.setOnFrameAvailableListener(this);
        mSTexture.setDefaultBufferSize(mPreviewSize.getWidth(), mPreviewSize.getHeight());

        Surface cameraOutputSurface = new Surface(mSTexture);

        GLES20.glClearColor(1.0f, 1.0f, 1.0f, 1.0f);

        Point ss = new Point();
        mView.getDisplay().getRealSize(ss);

        surfaceCreated(ss.x, ss.y, mPreviewSize.getWidth(), mPreviewSize.getHeight(),
                mCameraOrientation, mFocalLengthInPixels); // JNI
        mGLInit = true;
        mCameraManager.onSurfaceCreated(cameraOutputSurface);
    }

    // SurfaceTexture.OnFrameAvailableListener
    // Thread: "UI thread"
    @Override
    public void onFrameAvailable(SurfaceTexture st) {
        Log.d(TAG, "onFrameAvailable");
        if (st != mSTexture) {
            Log.e(TAG, "onFrameAvailable received for the wrong SurfaceTexture");
        }
        synchronized (this) { mUpdateST = true; }
        mView.requestRender(); // Can be called from any thread
    }

    private void initTex() {
        hTex = new int[1];
        GLES20.glGenTextures(1, hTex, 0);
        GLES20.glBindTexture(GLES11Ext.GL_TEXTURE_EXTERNAL_OES, hTex[0]);
        GLES20.glTexParameteri(GLES11Ext.GL_TEXTURE_EXTERNAL_OES, GLES20.GL_TEXTURE_WRAP_S, GLES20.GL_CLAMP_TO_EDGE);
        GLES20.glTexParameteri(GLES11Ext.GL_TEXTURE_EXTERNAL_OES, GLES20.GL_TEXTURE_WRAP_T, GLES20.GL_CLAMP_TO_EDGE);
        GLES20.glTexParameteri(GLES11Ext.GL_TEXTURE_EXTERNAL_OES, GLES20.GL_TEXTURE_MIN_FILTER, GLES20.GL_NEAREST);
        GLES20.glTexParameteri(GLES11Ext.GL_TEXTURE_EXTERNAL_OES, GLES20.GL_TEXTURE_MAG_FILTER, GLES20.GL_NEAREST);
    }

    private void sendMouseEvent(String type, float x, float y) {
        String str = type + ":" + x + "," + y;
        sendMessage(str);
    }

    private void sendMessage(String str) {
        try {
            // 创建一个InetAddree
            InetAddress serverAddress = InetAddress.getByName(Settings.ipnum);
            byte data[] = str.getBytes();
            // 创建一个DatagramPacket对象，并指定要讲这个数据包发送到网络当中的哪个地址，以及端口号
            DatagramPacket packet = new DatagramPacket(data, data.length, serverAddress, Settings.scoketnum);
            // 调用socket对象的send方法，发送数据
            socket.send(packet);
        } catch (Exception e) {
            // TODO Auto-generated catch block
            e.printStackTrace();
        }
    }

    // Native C++ methods
    private native void setAssetManager(AssetManager assetManager);
    private native void loadAsset(String name, String path);
    private native void surfaceCreated(int displayWidth, int displayHeight, int cameraWidth,
                                       int cameraHeight, int cameraOrientation, float cameraFocalLength);
    private native void surfaceChanged(int width, int height);
    private native double[] drawFrame(int texture);
    private native void destroy();
    private native int allocTexture(int width, int height);
}
