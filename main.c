/*

This example reads from the default PCM device
and writes to standard output for 5 seconds of data.

*/

/* Use the newer ALSA API */
#define ALSA_PCM_NEW_HW_PARAMS_API

#include <alsa/asoundlib.h>

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
    rc = snd_pcm_open(&handle, "default",
                        SND_PCM_STREAM_CAPTURE, 0);
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
    snd_pcm_hw_params_set_access(handle, params,
                          SND_PCM_ACCESS_RW_INTERLEAVED);

    /*PCM格式*/
    /* Signed 16-bit little-endian format */
    snd_pcm_hw_params_set_format(handle, params,
                                  SND_PCM_FORMAT_S16_LE);

    /*设置通道数*/
    /* Two channels (stereo) */
    snd_pcm_hw_params_set_channels(handle, params, 2);

    /*设置采样率*/
    /* 44100 bits/second sampling rate (CD quality) */
    val = 44100;
    snd_pcm_hw_params_set_rate_near(handle, params,
                                &val, &dir);

    /*没周期的帧数*/
    /* Set period size to 32 frames. */
    frames = 32;
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
    size = frames * 4; /* 2 bytes/sample, 2 channels */
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