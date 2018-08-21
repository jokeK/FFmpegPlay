//
// Created by Adim on 2018/8/14.
//
#include <pthread.h>
#include <android/log.h>

extern "C" {
#include <libavformat/avformat.h>
#include <libavdevice/avdevice.h>
#include <libswscale/swscale.h>
#include <libavutil/imgutils.h>
#include <libavutil/avutil.h>
#include <libavutil/time.h>
#include <libswresample/swresample.h>
}
#define LOGE(...)  __android_log_print(ANDROID_LOG_ERROR,"mik",__VA_ARGS__);

int createFfmpeg(int *rate, int *channel);

int getPcm(void **pcm,size_t *pcm_size);

void realseFFmpeg();

