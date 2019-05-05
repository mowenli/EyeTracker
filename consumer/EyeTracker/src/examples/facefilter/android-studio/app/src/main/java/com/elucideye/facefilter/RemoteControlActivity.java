package com.elucideye.facefilter;

import java.net.DatagramPacket;
import java.net.DatagramSocket;
import java.net.InetAddress;

import android.app.Activity;
import android.content.Intent;
import android.os.Bundle;
import android.util.Log;
import android.view.View;
import android.view.View.OnClickListener;
import android.widget.Button;
import android.widget.EditText;
import android.widget.Toast;

public class RemoteControlActivity extends Activity {
    EditText ipET;
    EditText socketET;
    Button button;
    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.main);
        ipET = (EditText)findViewById(R.id.IpEditText);
        socketET = (EditText)findViewById(R.id.SocketEditText);
        button = (Button)findViewById(R.id.ConnectButton);
        button.setOnClickListener(new OnClickListener() {
            @Override
            public void onClick(View v) {
                final String ipnum = ipET.getText().toString();
                int socketnum = Integer.parseInt(socketET.getText().toString());
                Settings.ipnum =ipnum;
                Settings.scoketnum = socketnum;

                new Thread(){
                    @Override
                    public void run()
                    {
                        try {
                            Log.i("~~~", "onClick: ");
                            // 创建DatagramSocket，实现数据发送和接收
                            DatagramSocket socket = new DatagramSocket();
                            // 创建一个InetAddree
                            InetAddress serverAddress = InetAddress.getByName(Settings.ipnum);
                            // 将字符串转换为字节数组
                            byte[] data = "connect to server: 1\n".getBytes();
                            // 创建数据报
                            DatagramPacket packet = new DatagramPacket(data, data.length, serverAddress, Settings.scoketnum);
                            // 向服务器端发送数据报
                            socket.send(packet);

                            //接收服务器响应数据
                            byte[] data2 = new byte[1024];
                            DatagramPacket packet2 = new DatagramPacket(data2, data2.length);
                            socket.receive(packet2);
                            String info = new String(data2, 0, packet2.getLength());
                            Log.i("~~~", "server response：" + info);
                            if (info.length() > 0) {
                                Intent intent = new Intent(RemoteControlActivity.this, FaceFilterActivity.class);
                                RemoteControlActivity.this.startActivity(intent);
                                RemoteControlActivity.this.finish();
//                                Toast.makeText(RemoteControlActivity.this, "连接成功", Toast.LENGTH_SHORT).show();
                            }
                        } catch (Exception e) {
                            // TODO Auto-generated catch block
                            e.printStackTrace();
                            Log.i("~~~", "onClick: " + e.toString());
                        }
                    }
                }.start();

            }
        });
    }
}