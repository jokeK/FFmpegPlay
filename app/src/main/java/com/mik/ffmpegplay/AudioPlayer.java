package com.mik.ffmpegplay;

import android.media.AudioFormat;
import android.media.AudioManager;
import android.media.AudioTrack;

/**
 * 播放音频
 */
public class AudioPlayer {
    static {
        System.loadLibrary("avcodec");
        System.loadLibrary("avdevice");
        System.loadLibrary("avfilter");
        System.loadLibrary("avformat");
        System.loadLibrary("avutil");
        System.loadLibrary("postproc");
        System.loadLibrary("swresample");
        System.loadLibrary("swscale");
        System.loadLibrary("native-lib");
    }

    public native void play();
    public native void stop();
    public native void pause();

//    public void playSound(final String input){
//        new Thread(new Runnable() {
//            @Override
//            public void run() {
//                sound(input);
//            }
//        }).start();
//    }
//    public native void sound(String input);
//    private AudioTrack audioTrack;
//    //sampleRateInHz采样率
//    public void createAudio(int sampleRateInHz,int nbChannels){
//        int channelConfig;
//        //通过通道数来判断
//        if (nbChannels == 2){
//            //立体声
//            channelConfig = AudioFormat.CHANNEL_OUT_STEREO;
//        }else {
//            channelConfig = AudioFormat.CHANNEL_OUT_MONO;
//        }
//        int bufferSize = AudioTrack.getMinBufferSize(sampleRateInHz,
//                channelConfig,AudioFormat.ENCODING_PCM_16BIT);
//        audioTrack = new AudioTrack(AudioManager.STREAM_MUSIC,sampleRateInHz,channelConfig,
//                AudioFormat.ENCODING_PCM_16BIT,bufferSize,AudioTrack.MODE_STREAM);
//        audioTrack.play();
//    }
//
//    public void playTrack(byte[] buffer,int length){
//        if (audioTrack!=null&&audioTrack.getPlayState()==AudioTrack.PLAYSTATE_PLAYING){
//            audioTrack.write(buffer,0,length);
//        }
//    }
}
