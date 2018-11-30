package com.example.stellasong.eyetracker;

import android.app.Activity;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.os.Bundle;
import android.view.View;
import android.widget.Button;
import android.widget.TextView;

import java.io.IOException;
import java.io.InputStream;

public class EyeTrackerActivity extends Activity {

    public EyetrackerNcnn eyetrackerNcnn = new EyetrackerNcnn();

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        setContentView(R.layout.activity_eyetracker);
        try {
            initSqueezeNcnn();
        } catch (IOException e) {
            e.printStackTrace();
        }

        ((Button) findViewById(R.id.buttonDetect)).setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View arg0) {
                Bitmap left_image = null, right_image = null, face_image = null;
                try {
                    InputStream is = getAssets().open("left_image.png");
                    left_image = BitmapFactory.decodeStream(is);
                    is.close();

                    is = getAssets().open("right_image.png");
                    right_image = BitmapFactory.decodeStream(is);
                    is.close();

                    is = getAssets().open("face_image.png");
                    face_image = BitmapFactory.decodeStream(is);
                    is.close();
                } catch (IOException e) {
                    e.printStackTrace();
                }

                int x = 10, y = 10;
                byte[] face_grid = new byte[x * 25 + y];
                if (left_image == null || right_image == null || face_image == null || face_grid == null)
                    return;

                String result = eyetrackerNcnn.Detect(left_image, right_image, face_image, face_grid);

                if (result == null)
                {
                    ((TextView) findViewById(R.id.infoResult)).setText("detect failed");
                }
                else
                {
                    ((TextView) findViewById(R.id.infoResult)).setText(result);
                }
            }
        });
    }

    private void initSqueezeNcnn() throws IOException
    {
        byte[] param = null;
        byte[] bin = null;

        {
            InputStream assetsInputStream = getAssets().open("eyetracker.param");
            int available = assetsInputStream.available();
            param = new byte[available];
            int byteCode = assetsInputStream.read(param);
            assetsInputStream.close();
        }
        {
            InputStream assetsInputStream = getAssets().open("eyetracker.bin");
            int available = assetsInputStream.available();
            bin = new byte[available];
            int byteCode = assetsInputStream.read(bin);
            assetsInputStream.close();
        }
        eyetrackerNcnn.Init(param, bin);
    }
}
