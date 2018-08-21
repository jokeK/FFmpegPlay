#include <jni.h>
#include <string>
#include <android/log.h>
#include <android/native_window_jni.h>
#include <pthread.h>
#include <SLES/OpenSLES.h>
#include <SLES/OpenSLES_Android.h>
#include "FFmpegMusic.h"

//extern "C" {
//#include <libavformat/avformat.h>
//#include <libavdevice/avdevice.h>
//#include <libswscale/swscale.h>
//#include <libavutil/imgutils.h>
//#include <libavutil/avutil.h>
//#include <libavutil/time.h>
//#include <libswresample/swresample.h>
//}
#define LOGE(...)  __android_log_print(ANDROID_LOG_ERROR,"mik",__VA_ARGS__);

extern "C"
JNIEXPORT void JNICALL
Java_com_mik_ffmpegplay_MainActivity_open(
        JNIEnv *env,
        jobject obj, jstring inputStr_, jstring outStr_) {
    const char *inputStr = env->GetStringUTFChars(inputStr_, 0);
    const char *outStr = env->GetStringUTFChars(outStr_, 0);
    //注册各大组件(新的API 已经不需要这一步了)
    av_register_all();
//    av_demuxer_iterate();
//    av_muxer_iterate()
    AVFormatContext *pContext = avformat_alloc_context();

    //参数ps包含一切媒体相关的上下文结构，有它就有了一切，本函数如果打开媒体成功，
    //会返回一个AVFormatContext的实例
    //参数fmt 是以什么格式打开文件
    //参数options是对某种格式的一些操作，是为了在命令行中可以对不同的格式传入
    //特殊的操作参数而建的， 为了了解流程，完全可以无视它

    if (avformat_open_input(&pContext, inputStr, NULL, NULL) < 0) {
        LOGE("视频文件打开失败");
        return;

    }
    if (avformat_find_stream_info(pContext, NULL) < 0) {
        LOGE("获取信息失败");
        return;
    }

    int video_stream_idx = -1;
    //视频信息的流中包含视频流，音频流等多种流
    //找到视频流 nb_streams流的数量
    for (int i = 0; i < pContext->nb_streams; ++i) {
        //codec 每一个流对应的解码上下文
        if (pContext->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO) {
            //pContext->streams[i]->codecpar
            video_stream_idx = i;
        }
    }
    //获取解码器上下文
    AVCodecContext *pCodeCtx = pContext->streams[video_stream_idx]->codec;
    //获得解码器
    AVCodec *pCodex = avcodec_find_decoder(pCodeCtx->codec_id);
    //解码
    if (avcodec_open2(pCodeCtx, pCodex, NULL) < 0) {
        LOGE("解码失败");
        return;
    }
    AVPacket *packet = static_cast<AVPacket *>(av_malloc(sizeof(AVPacket)));
    //AVPacket *packet = av_packet_alloc();
    //初始化结构体
    av_init_packet(packet);
    AVFrame *frame = av_frame_alloc();

    AVFrame *yuvFrame = av_frame_alloc();

    uint8_t *out_buffer = static_cast<uint8_t *>(av_malloc(avpicture_get_size(AV_PIX_FMT_YUV420P,
                                                                              pCodeCtx->width,
                                                                              pCodeCtx->height)));
    int re = avpicture_fill(reinterpret_cast<AVPicture *>(yuvFrame), out_buffer, AV_PIX_FMT_YUV420P,
                            pCodeCtx->width, pCodeCtx->height);
    //av_image_fill_arrays(yuvFrame->data,yuvFrame->linesize,out_buffer,AV_PIX_FMT_YUV420P, pCodeCtx->width, pCodeCtx->height,1);
    //编码转换上下文
    //SwsContext *swsContext = sws_alloc_context();
    SwsContext *swsContext = sws_getContext(pCodeCtx->width, pCodeCtx->height, pCodeCtx->pix_fmt,
                                            pCodeCtx->width, pCodeCtx->height, AV_PIX_FMT_YUV420P,
                                            SWS_BILINEAR, NULL, NULL, NULL);
    FILE *fp_yuv = fopen(outStr, "wb");

    int got_frame;
    while (av_read_frame(pContext, packet) >= 0) {
        //解封装 拿到视频的像素数据
        avcodec_decode_video2(pCodeCtx, frame, &got_frame, packet);
        if (got_frame > 0) {
            //转换每一帧的格式为YUV
            sws_scale(swsContext, reinterpret_cast<const uint8_t *const *>(frame->data),
                      frame->linesize, 0,
                      frame->height, yuvFrame->data, yuvFrame->linesize);
        }
        //一帧的大小  YUV 是根据人眼结构设计的，这里亮度 色彩 浓度  4：1：1的关系
        int y_size = pCodeCtx->height * pCodeCtx->width;
        fwrite(yuvFrame->data[0], 1, y_size, fp_yuv);
        fwrite(yuvFrame->data[1], 1, y_size / 4, fp_yuv);
        fwrite(yuvFrame->data[2], 1, y_size / 4, fp_yuv);

    }
    av_free_packet(packet);
    fclose(fp_yuv);
    av_frame_free(&frame);
    av_frame_free(&yuvFrame);
    avcodec_close(pCodeCtx);
    avformat_free_context(pContext);

    env->ReleaseStringUTFChars(inputStr_, inputStr);
    env->ReleaseStringUTFChars(outStr_, outStr);

}

extern "C"
JNIEXPORT void JNICALL
Java_com_mik_ffmpegplay_VideoView_render(JNIEnv *env, jobject instance, jstring input_,
                                         jobject surface) {
    const char *input = env->GetStringUTFChars(input_, 0);

    av_register_all();
//    av_demuxer_iterate();
//    av_muxer_iterate()
    AVFormatContext *pContext = avformat_alloc_context();

    if (avformat_open_input(&pContext, input, NULL, NULL) < 0) {
        LOGE("视频文件打开失败");
        return;
    }
    if (avformat_find_stream_info(pContext, NULL) < 0) {
        LOGE("获取信息失败");
        return;
    }
    int video_stream_idx = -1;
    //视频信息的流中包含视频流，音频流等多种流
    //找到视频流 nb_streams流的数量
    for (int i = 0; i < pContext->nb_streams; ++i) {
        //codec 每一个流对应的解码上下文
        if (pContext->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO) {
            //pContext->streams[i]->codecpar
            video_stream_idx = i;
        }
    }
    //获取解码器上下文
    AVCodecContext *pCodeCtx = pContext->streams[video_stream_idx]->codec;
    //获得解码器
    AVCodec *pCodex = avcodec_find_decoder(pCodeCtx->codec_id);

    if (avcodec_open2(pCodeCtx, pCodex, NULL) < 0) {
        LOGE("解码失败");
        return;
    }
    AVPacket *packet = static_cast<AVPacket *>(av_malloc(sizeof(AVPacket)));
    //AVPacket *packet = av_packet_alloc();
    //初始化结构体
    av_init_packet(packet);
    AVFrame *frame = av_frame_alloc();
    AVFrame *rgb_frame = av_frame_alloc();
    //给缓冲区分配内存
    uint8_t *out_buffer = static_cast<uint8_t *>(av_malloc(avpicture_get_size(AV_PIX_FMT_RGBA,
                                                                              pCodeCtx->width,
                                                                              pCodeCtx->height)));
    int re = avpicture_fill(reinterpret_cast<AVPicture *>(rgb_frame), out_buffer, AV_PIX_FMT_RGBA,
                            pCodeCtx->width, pCodeCtx->height);
    //av_image_fill_arrays(rgb_frame->data,rgb_frame->linesize,out_buffer,AV_PIX_FMT_RGBA,pCodeCtx->width, pCodeCtx->height,1);
    SwsContext *swsContext = sws_getContext(pCodeCtx->width, pCodeCtx->height, pCodeCtx->pix_fmt,
                                            pCodeCtx->width, pCodeCtx->height, AV_PIX_FMT_RGBA,
                                            SWS_BILINEAR, NULL, NULL, NULL);

    int got_frame;
    ANativeWindow *nativeWindow = ANativeWindow_fromSurface(env, surface);
    //视频缓冲区
    ANativeWindow_Buffer window_buffer;
    while (av_read_frame(pContext, packet) >= 0) {
        //判断是否是视频，
        // 这里不要用 != video_stream_idx 然后return 逻辑来做
        //stream_index里面还有音频流
        if (packet->stream_index == video_stream_idx) {
            //拿到视频的像素数据 解码
            avcodec_decode_video2(pCodeCtx, frame, &got_frame, packet);
            if (got_frame) {
                ANativeWindow_setBuffersGeometry(nativeWindow, pCodeCtx->width, pCodeCtx->height,
                                                 WINDOW_FORMAT_RGBA_8888);
                //绘制开始
                //先锁定画布 坑点：&window_buffer 不能传*指针
                //不然会报一个 too mach works on main thread 的错误 很莫名其妙
                ANativeWindow_lock(nativeWindow, &window_buffer, NULL);
                //转为指定格式的
                sws_scale(swsContext, reinterpret_cast<const uint8_t *const *>(frame->data),
                          frame->linesize, 0,
                          frame->height, rgb_frame->data, rgb_frame->linesize);
                //拿到window画布的首地址
                //void *dst = window_buffer->bits;
                //The actual bits 实际的位置
                uint8_t *dst = static_cast<uint8_t *>(window_buffer.bits);
                //拿到一行有多少个像素  ARGBS所以*4
                //内存地址
                int destStride = window_buffer.stride * 4;
                //像素数据的首地址
                uint8_t *src = rgb_frame->data[0];
                //实际数据在内存中一行的数量
                int srcStride = rgb_frame->linesize[0];
                for (int i = 0; i < pCodeCtx->height; ++i) {
                    memcpy(dst + i * destStride, src + i * srcStride, srcStride);
                }
                ANativeWindow_unlockAndPost(nativeWindow);
                //保证每秒60帧
                av_usleep(1000 * 16);//微秒
            }
        }

    }
    av_free_packet(packet);
    ANativeWindow_release(nativeWindow);
    av_frame_free(&frame);
    av_frame_free(&rgb_frame);
    avcodec_close(pCodeCtx);
    avformat_free_context(pContext);

    env->ReleaseStringUTFChars(input_, input);
}
extern "C" JNIEXPORT void JNICALL
Java_com_mik_ffmpegplay_AudioPlayer_sound(JNIEnv *env, jobject instance, jstring input_) {

    const char *input = env->GetStringUTFChars(input_, 0);
    jclass audio_player = env->GetObjectClass(instance);
    jmethodID create_audio_methodID = env->GetMethodID(audio_player, "createAudio", "(II)V");
    jmethodID audio_player_methodID = env->GetMethodID(audio_player, "playTrack", "([BI)V");

    av_register_all();
//    av_demuxer_iterate();
//    av_muxer_iterate()
    AVFormatContext *pContext = avformat_alloc_context();

    if (avformat_open_input(&pContext, input, NULL, NULL) < 0) {
        LOGE("音频文件打开失败");
        return;

    }
    if (avformat_find_stream_info(pContext, NULL) < 0) {
        LOGE("获取信息失败");
        return;
    }

    int audio_stream_idx = -1;
    //视频信息的流中包含视频流，音频流等多种流
    //找到音频流 nb_streams流的数量
    for (int i = 0; i < pContext->nb_streams; ++i) {
        //codec 每一个流对应的解码上下文
        if (pContext->streams[i]->codec->codec_type == AVMEDIA_TYPE_AUDIO) {
            //pContext->streams[i]->codecpar
            //找到音频流
            audio_stream_idx = i;
        }
    }
    //获取音频解码器上下文
    AVCodecContext *pCodeCtx = pContext->streams[audio_stream_idx]->codec;

    //获得解码器
    AVCodec *pCodex = avcodec_find_decoder(pCodeCtx->codec_id);

    if (avcodec_open2(pCodeCtx, pCodex, NULL) < 0) {
        LOGE("解码失败");
        return;
    }
    AVPacket *packet = static_cast<AVPacket *>(av_malloc(sizeof(AVPacket)));
    //AVPacket *packet = av_packet_alloc();
    //初始化结构体
    av_init_packet(packet);
    AVFrame *frame = av_frame_alloc();
    SwrContext *swrContext = swr_alloc();
    int got_grame;

    //输出通道布局 AV_CH_LAYOUT_STEREO 立体声
    int64_t out_ch_layout = AV_CH_LAYOUT_STEREO;
    //输出采样率位数 16位
    enum AVSampleFormat out_format = AV_SAMPLE_FMT_S16;
    //采样率  输出采样率必须和输入采样率一致
    int out_sample_rate = pCodeCtx->sample_rate;
    //设置音频转换参数
    swr_alloc_set_opts(swrContext, out_ch_layout, out_format, out_sample_rate,
                       pCodeCtx->channel_layout, pCodeCtx->sample_fmt, pCodeCtx->sample_rate, 0,
                       NULL);

    swr_init(swrContext);

    //采样率44100，双通道16位 8位一个字节，两个字节
    //音频所用内存字节数 = (通道数*采样频率(HZ)*采样位数(byte))÷8
    //按道理应该是4100*2*2  少*2也没问题，能播放
    uint8_t *out_buffer = static_cast<uint8_t *>(av_malloc(44100 * 2));

    //获取音频的通道数
    int out_channel_nb = av_get_channel_layout_nb_channels(AV_CH_LAYOUT_STEREO);
    //反射调用java方法
    env->CallVoidMethod(instance, create_audio_methodID, 44100, out_channel_nb);

    while (av_read_frame(pContext, packet) >= 0) {
        if (packet->stream_index == audio_stream_idx) {
            //解码
            avcodec_decode_audio4(pCodeCtx, frame, &got_grame, packet);
            if (got_grame) {
                //开始解码
                swr_convert(swrContext, &out_buffer, 44100 * 2,
                            (const uint8_t **) frame->data, frame->nb_samples);
                //AV_SAMPLE_FMT_S16 要和输入的格式一致
                int size = av_samples_get_buffer_size(NULL, out_channel_nb,
                                                      frame->nb_samples, AV_SAMPLE_FMT_S16, 0);
                jbyteArray audio_samples_array = env->NewByteArray(size);
                env->SetByteArrayRegion(audio_samples_array, 0, size,
                                        reinterpret_cast<const jbyte *>(out_buffer));
                env->CallVoidMethod(instance, audio_player_methodID, audio_samples_array, size);
                env->DeleteLocalRef(audio_samples_array);
            }
        }

    }
    av_frame_free(&frame);
    swr_free(&swrContext);
    avcodec_close(pCodeCtx);
    // avformat_free_context(pContext);
    avformat_close_input(&pContext);

    env->ReleaseStringUTFChars(input_, input);

}
/**
 * OpenSL_ES播放音频
 */
SLObjectItf engineObject = NULL;
SLEngineItf engineItf = NULL;
//混音器
SLObjectItf outputMixObject = NULL;
SLEnvironmentalReverbItf outputEnvironmentalReverbItf =NULL;

SLEnvironmentalReverbSettings settings = SL_I3DL2_ENVIRONMENT_PRESET_DEFAULT;

//播放器
SLObjectItf playObject=NULL;
SLPlayItf playItf =NULL;
//队列缓冲区
SLBufferQueueItf playerQueue =NULL;

//音量对象
SLVolumeItf volumeItf =NULL;

//只要喇叭读完PCM数据就会回调此函数
//添加PCM数据到缓冲区
void *buffer;
size_t buffer_size = 0;

void playerCallBack(SLBufferQueueItf queueItf, void *context) {
    buffer_size = 0;
    //取到音频数据
    getPcm(&buffer, &buffer_size);
    if (buffer != NULL && buffer_size != 0) {
        //播放的关键地方
        SLresult result = (*playerQueue)->Enqueue(playerQueue,buffer,buffer_size);
        LOGE("正在播放 %d",result);
    }
}

extern "C" JNIEXPORT void JNICALL
Java_com_mik_ffmpegplay_AudioPlayer_play(JNIEnv *env, jobject instance) {
    //初始化OpenSL_ES引擎
    slCreateEngine(&engineObject, 0, NULL, 0, NULL, NULL);
    /**
     * 初始化状态
     * SL_BOOLEAN_FALSE 代表同步
     */
    (*engineObject)->Realize(engineObject, SL_BOOLEAN_FALSE);
    /**
     * 实例化引擎接口
     * 第二个参数是接口的ID
     */
    (*engineObject)->GetInterface(engineObject, SL_IID_ENGINE, &engineItf);

    //初始化FFmpeg
    int rate;
    int channel;
    createFfmpeg(&rate,&channel);
    LOGE("FFmpeg初始化完毕 %d",channel);
    /**
     * 创建混音器
     */
    SLresult sLresult;
    (*engineItf)->CreateOutputMix(engineItf, &outputMixObject, 0, 0, 0);
    (*outputMixObject)->Realize(outputMixObject, SL_BOOLEAN_FALSE);
    //设置环境混响
    sLresult = (*outputMixObject)->GetInterface(outputMixObject, SL_IID_ENVIRONMENTALREVERB,
                                                &outputEnvironmentalReverbItf);

    if (sLresult == SL_RESULT_SUCCESS) {
        (*outputEnvironmentalReverbItf)->
                SetEnvironmentalReverbProperties(outputEnvironmentalReverbItf, &settings);
    }

    //设置混音器
    SLDataLocator_OutputMix outputMix = {SL_DATALOCATOR_OUTPUTMIX, outputMixObject};
    //混音器关联
    SLDataSink slDataSink = {&outputMix, NULL};


    //为什么播放器缓冲区数量是2(numBuffers)
    //当一个在播放的时候可以在另一个填充新的数据，增加缓冲区的数量时
    //也增加了延迟（数据从加入缓冲区到播放缓冲区的时间）
    SLDataLocator_AndroidSimpleBufferQueue bufferQueue = {SL_DATALOCATOR_ANDROIDSIMPLEBUFFERQUEUE, 2};
    /**
     * typedef struct SLDataFormat_PCM_ {
	SLuint32 		formatType; pcm
	SLuint32 		numChannels; 通道数
	SLuint32 		samplesPerSec; 采样率
	SLuint32 		bitsPerSample; 采样位数
	SLuint32 		containerSize; 包含位数
	SLuint32 		channelMask;  立体声 左声道 右声道
	SLuint32		endianness; 结束标志位
} SLDataFormat_PCM;
     */
    SLDataFormat_PCM pcm = {SL_DATAFORMAT_PCM, static_cast<SLuint32>(channel), SL_SAMPLINGRATE_44_1,
                            SL_PCMSAMPLEFORMAT_FIXED_16, SL_PCMSAMPLEFORMAT_FIXED_16,
                            SL_SPEAKER_FRONT_LEFT | SL_SPEAKER_FRONT_RIGHT,
                            SL_BYTEORDER_LITTLEENDIAN};

    SLDataSource slDataSource = {&bufferQueue, &pcm};

    SLInterfaceID ids[3] = {SL_IID_BUFFERQUEUE, SL_IID_EFFECTSEND, SL_IID_VOLUME};

    SLboolean req[3] = {SL_BOOLEAN_TRUE, SL_BOOLEAN_TRUE, SL_BOOLEAN_TRUE};


    //URI
//    SLObjectItf outputMixObject = NULL;
//    SLEnvironmentalReverbItf outputMixEnvironmentalReverb = NULL;////创建具体的混音器对象实例
//    SLEnvironmentalReverbSettings reverbSettings =SL_I3DL2_ENVIRONMENT_PRESET_DEFAULT;
//    SLInterfaceID mids[1] = {SL_IID_ENVIRONMENTALREVERB};
//    SLboolean mreq[1] = {SL_BOOLEAN_FALSE};
//    //设置混音器
//    (*engineItf)->CreateOutputMix(engineItf,&outputMixObject,1,mids,mreq);
//    (*outputMixObject)->Realize(outputMixObject,SL_BOOLEAN_FALSE);
//    int result;
//    result = (*outputMixObject)->GetInterface(outputMixObject, SL_IID_ENVIRONMENTALREVERB,
//                                              &outputMixEnvironmentalReverb);
//    if (SL_RESULT_SUCCESS == result) {
//        result = (*outputMixEnvironmentalReverb)->SetEnvironmentalReverbProperties(outputMixEnvironmentalReverb, &reverbSettings);
//
//    }
//
//    // configure audio sink
//
//    SLDataLocator_OutputMix loc_outmix = {SL_DATALOCATOR_OUTPUTMIX, outputMixObject};
//
//    SLDataSink audioSnk = {&loc_outmix, NULL};
//
//
//    SLDataLocator_URI loc_uri = {SL_DATALOCATOR_URI, (SLchar *) "链接"};
//
//    SLDataFormat_MIME format_mime = {SL_DATAFORMAT_MIME, NULL, SL_CONTAINERTYPE_UNSPECIFIED};
//
//    SLDataSource audioSrc = {&loc_uri, &format_mime};


    /**
     * SLresult (*CreateAudioPlayer) (
		SLEngineItf self,
		SLObjectItf * pPlayer,
		SLDataSource *pAudioSrc,
		SLDataSink *pAudioSnk,
		SLuint32 numInterfaces,
		const SLInterfaceID * pInterfaceIds,
		const SLboolean * pInterfaceRequired
	);
     */
    //创建播放器
    (*engineItf)->CreateAudioPlayer(engineItf, &playObject, &slDataSource, &slDataSink, 3, ids,
                                    req);
    (*playObject)->Realize(playObject, SL_BOOLEAN_FALSE);
    (*playObject)->GetInterface(playObject, SL_IID_PLAY, &playItf);

    //注册缓冲区队列
    (*playObject)->GetInterface(playObject, SL_IID_BUFFERQUEUE, &playerQueue);

    //设置回调接口
    (*playerQueue)->RegisterCallback(playerQueue,playerCallBack,NULL);

    //初始化音量对象
    (*playObject)->GetInterface(playObject,SL_IID_VOLUME,&volumeItf);
    //设置播放状态
    (*playItf)->SetPlayState(playItf,SL_PLAYSTATE_PLAYING);

    //播放第一帧
    playerCallBack(playerQueue,NULL);
}


extern "C" JNIEXPORT void JNICALL
Java_com_mik_ffmpegplay_AudioPlayer_stop(JNIEnv *env, jobject instance) {
    (*playItf)->SetPlayState(playItf,SL_PLAYSTATE_STOPPED);
    //释放
    if (playObject!=NULL){
        (*playObject)->Destroy(playObject);
        playObject = NULL;
        playItf=NULL;
        playerQueue =NULL;
        volumeItf =NULL;
    }

    if (outputMixObject!=NULL){
        (*outputMixObject)->Destroy(outputMixObject);
        outputMixObject = NULL;
        playItf = NULL;
    }
    if (engineObject!=NULL){
        (*engineObject)->Destroy(engineObject);
        engineObject = NULL;
        engineItf = NULL;
    }
    realseFFmpeg();

}
extern "C" JNIEXPORT void JNICALL
Java_com_mik_ffmpegplay_AudioPlayer_pause(JNIEnv *env, jobject instance) {

    (*playItf)->SetPlayState(playItf,SL_PLAYSTATE_PAUSED);

}
