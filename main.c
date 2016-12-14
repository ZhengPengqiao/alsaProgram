/*

This example reads from the default PCM device
and writes to standard output for 5 seconds of data.
*/

/* Use the newer ALSA API */
#define ALSA_PCM_NEW_HW_PARAMS_API


/*
样本长度(sample)：样本是记录音频数据最基本的单位，常见的有8位和16位。

通道数(channel)：该参数为1表示单声道，2则是立体声。

桢(frame)：桢记录了一个声音单元，其长度为样本长度与通道数的乘积。

采样率(rate)：每秒钟采样次数，该次数是针对桢而言。

周期(period)：音频设备一次处理所需要的桢数，对于音频设备的数据访问以及音频数据的存储，都是以此为单
位。

交错模式(interleaved)：是一种音频数据的记录方式，在交错模式下，数据以连续桢的形式存放，即首先记录
完桢1的左声道样本和右声道样本（假设为立体声格式），再开始桢2的记录。而在非交错模式下，首先记录的是一
个周期内所有桢的左声道样本，再记录右声道样本，数据是以连续通道的方式存储。不过多数情况下，我们只需要
使用交错模式就可以了。


period(周期):硬件中中断间的间隔时间。它表示输入延时。
声卡接口中有一个指针来指示声卡硬件缓存区中当前的读写位置。只要接口在运行，这个指针将循环地指向缓存区
中的某个位置。
frame size = sizeof(one sample) * nChannels
alsa中配置的缓存(buffer)和周期(size)大小在runtime中是以帧(frames)形式存储的。
period_bytes = frames_to_bytes(runtime, runtime->period_size);
bytes_to_frames()

The period and buffer sizes are not dependent on the sample format because they are measured in frames; you do not need to change them.
*/
#include <alsa/asoundlib.h>

#define    M_AUDIO_CHANNELS        1       //音频通道数量，必须固定是1，不然代码需要修改
#define    M_AUDIO_SAMPLERATE      8000    //音频采样频率，11025、22050、44100等
#define    M_AUDIO_BITSPERSAMPLE   SND_PCM_FORMAT_S16_LE//音频样本位数，必须固定是16
#define    FRAMES                  505           //每周期的帧数

int main() {
    long loops;
    int rc;
    int size;
    snd_pcm_t *handle;
    snd_pcm_hw_params_t *params;
    unsigned int val;
    int dir;
    snd_pcm_uframes_t frames;
    char *buffer;

    /*以录制模式打开*/
    /* Open PCM device for recording (capture). */
    rc = snd_pcm_open(&handle, "default", SND_PCM_STREAM_CAPTURE, 0);
    if (rc < 0) {
        fprintf(stderr,
                "unable to open pcm device: %s/n",
                snd_strerror(rc));
        exit(1);
    }

    /*分配一个参数对象*/
    /* Allocate a hardware parameters object. */
    snd_pcm_hw_params_alloca(&params);

    /*初始化参数对象*/
    /* Fill it in with default values. */
    snd_pcm_hw_params_any(handle, params);

    /* Set the desired hardware parameters. */

    /*交错模式*/
    /* Interleaved mode */
    //snd_pcm_hw_params_set_access(handle, params, \
                          SND_PCM_ACCESS_RW_INTERLEAVED);

    /*PCM格式 M_AUDIO_BITSPERSAMPLE*/
    snd_pcm_hw_params_set_format(handle, params,
                                  M_AUDIO_BITSPERSAMPLE);

    /*设置通道数M_AUDIO_CHANNELS*/
    snd_pcm_hw_params_set_channels(handle, params, M_AUDIO_CHANNELS);

    /*设置采样率*/
    val = M_AUDIO_SAMPLERATE;
    snd_pcm_hw_params_set_rate_near(handle, params,
                                &val, &dir);

    /*每周期的帧数*/
    frames = FRAMES;
    snd_pcm_hw_params_set_period_size_near(handle,
                            params, &frames, &dir);

    /* Write the parameters to the driver */
    rc = snd_pcm_hw_params(handle, params);
    if (rc < 0) {
        fprintf(stderr,
                "unable to set hw parameters: %s/n",
                snd_strerror(rc));
        exit(1);
    }

    /* Use a buffer large enough to hold one period */
    snd_pcm_hw_params_get_period_size(params,
                                          &frames, &dir);
    /* 2 bytes/sample,  M_AUDIO_CHANNELS channels */
    size = frames * 2 * M_AUDIO_CHANNELS; 
    buffer = (char *) malloc(size);

    /* We want to loop for 5 seconds */
    snd_pcm_hw_params_get_period_time(params, &val, &dir);
    loops = 5000000 / val;

    while (loops > 0) {
        loops--;
        rc = snd_pcm_readi(handle, buffer, frames);
        if (rc == -EPIPE) {
          /* EPIPE means overrun */
          fprintf(stderr, "overrun occurred/n");
          //把PCM流置于PREPARED状态，这样下次我们向该PCM流中数据时，它就能重新开始处理数据。
          snd_pcm_prepare(handle);
        } else if (rc < 0) {
          fprintf(stderr,
                  "error from read: %s/n",
                  snd_strerror(rc));
        } else if (rc != (int)frames) {
          fprintf(stderr, "short read, read %d frames/n", rc);
        }
        rc = write(1, buffer, size);
        if (rc != size)
          fprintf(stderr,
                  "short write: wrote %d bytes/n", rc);
    }

    //调用snd_pcm_drain把所有挂起没有传输完的声音样本传输完全
    snd_pcm_drain(handle);
    //关闭该音频流，释放之前动态分配的缓冲区，退出
    snd_pcm_close(handle);
    free(buffer);

    return 0;
}