package com.mik.ffmpegplay;

import android.view.Surface;

/**
 * 音视频同步
 */
public class MikPlayer {
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

    public native void play(String path);

    public native void disPlay(Surface surface);

    public native void release();

}
