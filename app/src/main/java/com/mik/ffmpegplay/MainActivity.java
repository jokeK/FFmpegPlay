package com.mik.ffmpegplay;

import android.Manifest;
import android.content.pm.PackageManager;
import android.graphics.BitmapFactory;
import android.os.Bundle;
import android.os.Environment;
import android.support.v4.app.ActivityCompat;
import android.support.v4.content.ContextCompat;
import android.support.v7.app.AppCompatActivity;
import android.view.View;
import android.widget.ArrayAdapter;
import android.widget.Spinner;

import java.io.File;

public class MainActivity extends AppCompatActivity {

    // Used to load the 'native-lib' library on application startup.
    VideoView videoView;
    Spinner sp_video;
    AudioPlayer player;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
        requestAllPower();
        videoView = findViewById(R.id.surface);
        sp_video = findViewById(R.id.sp_video);
        String[] array = getResources().getStringArray(R.array.video_list);
        ArrayAdapter<String> adapter = new ArrayAdapter<>(this, android.R.layout.simple_list_item_1, android.R.id.text1, array);
        sp_video.setAdapter(adapter);

    }

    //public native void open(String inputPath,String outPath);


//    public void load(View view) {
//        String inputPath = new File(Environment.getExternalStorageDirectory(),"mik.mp4").getAbsolutePath();
//        String outPath = new File(Environment.getExternalStorageDirectory(),"output.yuv").getAbsolutePath();
//        open(inputPath,outPath);
//    }

    public void mPlay(View view) {
        String video = sp_video.getSelectedItem().toString();
        String path = new File(Environment.getExternalStorageDirectory(), video).getAbsolutePath();
        videoView.player(path);

    }
    public void audioPlay(View view) {
        String input = new File(Environment.getExternalStorageDirectory(), "audio.mp3").getAbsolutePath();
        player = new AudioPlayer();
        player.play();
        //player.playSound(input);
    }

    public void stop(View view) {
        player.stop();
    }
    public void pause(View view) {
        player.pause();
    }

    public void requestAllPower() {
        if (ContextCompat.checkSelfPermission(this,
                Manifest.permission.WRITE_EXTERNAL_STORAGE)
                != PackageManager.PERMISSION_GRANTED) {
            if (ActivityCompat.shouldShowRequestPermissionRationale(this,
                    Manifest.permission.WRITE_EXTERNAL_STORAGE)) {
            } else {
                ActivityCompat.requestPermissions(this,
                        new String[]{Manifest.permission.WRITE_EXTERNAL_STORAGE,
                                Manifest.permission.READ_EXTERNAL_STORAGE,Manifest.permission.RECORD_AUDIO}, 10);
            }
        }
    }



}
