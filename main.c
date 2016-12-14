/*

This example reads standard from input and writes
to the default PCM device for 5 seconds of data.

*/

/* Use the newer ALSA API */
#define ALSA_PCM_NEW_HW_PARAMS_API

#include <alsa/asoundlib.h>

#define    M_AUDIO_CHANNELS        1       //音频通道数量，必须固定是1，不然代码需要修改
#define    M_AUDIO_SAMPLERATE      44100    //音频采样频率，11025、22050、44100等
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

    /* Open PCM device for playback. */
    rc = snd_pcm_open(&handle, "default",
                    SND_PCM_STREAM_PLAYBACK, 0);
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
    //snd_pcm_hw_params_set_access(handle, params,\
                      SND_PCM_ACCESS_RW_INTERLEAVED);

    /*设置PCM格式*/
    /* Signed 16-bit little-endian format */
    snd_pcm_hw_params_set_format(handle, params,
                              M_AUDIO_BITSPERSAMPLE);

    /*设置通道数*/
    snd_pcm_hw_params_set_channels(handle, params, M_AUDIO_CHANNELS);

    /*设置采样率*/
    val = M_AUDIO_SAMPLERATE;
    snd_pcm_hw_params_set_rate_near(handle, params,
                                  &val, &dir);

    /* Set period size to FRAMES frames. */
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
    snd_pcm_hw_params_get_period_size(params, &frames, &dir);
    size = frames * 2 * M_AUDIO_CHANNELS; /* 2 bytes/sample, 2 channels */
    buffer = (char *) malloc(size);

    /* We want to loop for 5 seconds */
    snd_pcm_hw_params_get_period_time(params,
                                    &val, &dir);
    /* 5 seconds in microseconds divided by
    * period time */
    loops = 40000000 / val;

    while (loops > 0) {
        loops--;
        rc = read(0, buffer, size);
        if (rc == 0) {
            fprintf(stderr, "end of file on input/n");
            break;
        } else if (rc != size) {
            fprintf(stderr,
                  "short read: read %d bytes/n", rc);
        }
        rc = snd_pcm_writei(handle, buffer, frames);
        if (rc == -EPIPE) {
            /* EPIPE means underrun */
            fprintf(stderr, "underrun occurred/n");
            //把PCM流置于PREPARED状态，这样下次我们向该PCM流中数据时，它就能重新开始处理数据。
            snd_pcm_prepare(handle); 
        } else if (rc < 0) {
            fprintf(stderr,
                "error from writei: %s/n",
            snd_strerror(rc));
        }  else if (rc != (int)frames) {
            fprintf(stderr,
                  "short write, write %d frames/n", rc);
        }
    }

    //调用snd_pcm_drain把所有挂起没有传输完的声音样本传输完全
    snd_pcm_drain(handle);
    //关闭该音频流，释放之前动态分配的缓冲区，退出
    snd_pcm_close(handle);
    free(buffer);

  return 0;
}