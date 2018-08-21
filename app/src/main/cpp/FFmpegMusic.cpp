//
// Created by Adim on 2018/8/14.
//

#include "FFmpegMusic.h"
AVFormatContext *pContext;
AVCodecContext *pCodeCtx;
AVCodec *pCodex;
AVPacket *packet;
AVFrame *frame;
SwrContext *swrContext;
uint8_t* out_buffer;
int out_channel_nb;
int audio_stream_idx = -1;
/**
 *
 * @param rate 采样率
 * @param channel 通道数
 */
int createFfmpeg(int *rate, int *channel){
    av_register_all();
//    av_demuxer_iterate();
//    av_muxer_iterate()
    pContext = avformat_alloc_context();
    char *input = "/sdcard/input.mp3";
    if (avformat_open_input(&pContext, input, NULL, NULL)<0) {
        LOGE("音频文件打开失败");
        return -1;

    }
    if (avformat_find_stream_info(pContext, NULL) < 0) {
        LOGE("获取信息失败");
        return -1;
    }


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
    pCodeCtx = pContext->streams[audio_stream_idx]->codec;

    //获得解码器
    pCodex = avcodec_find_decoder(pCodeCtx->codec_id);

    if (avcodec_open2(pCodeCtx, pCodex, NULL) < 0) {
        LOGE("解码失败");
        return -1;
    }
    packet = static_cast<AVPacket *>(av_malloc(sizeof(AVPacket)));
    //AVPacket *packet = av_packet_alloc();
    //初始化结构体
    av_init_packet(packet);
    frame = av_frame_alloc();
    swrContext = swr_alloc();
    int got_grame;

    //输出通道布局 AV_CH_LAYOUT_STEREO 立体声
    int64_t out_ch_layout = AV_CH_LAYOUT_STEREO;
    //输出采样率位数 16位
    enum AVSampleFormat out_format = AV_SAMPLE_FMT_S16;
    //采样率  输出采样率必须和输入采样率一致
    int out_sample_rate = pCodeCtx->sample_rate;
    //设置音频转换参数
    swr_alloc_set_opts(swrContext,out_ch_layout,out_format,out_sample_rate,
                       pCodeCtx->channel_layout,pCodeCtx->sample_fmt,pCodeCtx->sample_rate,0,NULL);

    swr_init(swrContext);

    //采样率44100，双通道16位 8位一个字节，两个字节
    //音频所用内存字节数 = (通道数*采样频率(HZ)*采样位数(byte))÷8
    //按道理应该是4100*2*2  少*2也没问题，能播放
    out_buffer = static_cast<uint8_t *>(av_malloc(44100 * 2));

    //获取音频的通道数
    out_channel_nb = av_get_channel_layout_nb_channels(AV_CH_LAYOUT_STEREO);
    *rate = pCodeCtx->sample_rate;
    *channel = pCodeCtx->channels;
    return 0;
}
/**
 *
 * @param pcm 缓冲区数组
 * @param size 缓冲区大小
 * @return
 */
int getPcm(void **pcm,size_t *pcm_size){
    while (av_read_frame(pContext,packet)>=0){
        int got_grame;
        if (packet->stream_index == audio_stream_idx){
            //解码
            avcodec_decode_audio4(pCodeCtx,frame,&got_grame,packet);
            if (got_grame){
                //开始解码
                swr_convert(swrContext, &out_buffer,44100*2,
                            (const uint8_t **)frame->data, frame->nb_samples);
                //AV_SAMPLE_FMT_S16 要和输入的格式一致
                //缓冲区的大小
                int size = av_samples_get_buffer_size(NULL,out_channel_nb,
                                                      frame->nb_samples,AV_SAMPLE_FMT_S16,0);
                *pcm = out_buffer;
                *pcm_size = size;
                break;
            }
        }

    }
    return 0;
}

void realseFFmpeg(){
    av_free_packet(packet);
    av_frame_free(&frame);
    swr_free(&swrContext);
    av_free(out_buffer);
    avcodec_close(pCodeCtx);
    // avformat_free_context(pContext);
    avformat_close_input(&pContext);

}

