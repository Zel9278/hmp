#include <stdio.h>
#include <math.h>
#include <pthread.h>
#include <unistd.h>

#include "midi_header.h"
#include "midi_data.h"
#include "midiplayer.h"
#include "thread_args.h"
#define TSF_IMPLEMENTATION
#include "tsf.h"

void *loop_tsf(void *context)
{
    struct loop_args *args = (struct loop_args *)context;
    tsf *TinySoundFont = args->TinySoundFont;
    int sample_rate = args->sample_rate;

    int buffer_length_ms = 1;
    int sample_count = (sample_rate * buffer_length_ms) / 1000;
    float buffer[sample_count * 2];

    while (1)
    {
        tsf_render_float(TinySoundFont, buffer, sample_count, 0);

        fwrite(buffer, sizeof(float), sample_count * 2, stdout);

        /*
                int delay_us = (sample_count * 1000000) / sample_rate;
                usleep(delay_us);
        */
        usleep(1);
    }

    return NULL;
}

void *initTSF(void *context)
{
    struct initTSF_args *args = (struct initTSF_args *)context;
    char *soundfont = args->soundfont;
    char *midiFile = args->midiFile;

    pthread_t midiPlayer_load_thread;
    int midiPlayer_load_result = 0;
    pthread_t loopTSF_thread;
    int loopTSF_result = 0;

    tsf *TinySoundFont = tsf_load_filename(soundfont);

    int sample_rate = 48000;
    tsf_set_output(TinySoundFont, TSF_STEREO_INTERLEAVED, sample_rate, 0);
    tsf_set_max_voices(TinySoundFont, 1000);

    struct midiPlayer_load_args *mp_args = malloc(sizeof(struct midiPlayer_load_args));
    mp_args->TinySoundFont = TinySoundFont;
    mp_args->midiFile = midiFile;

    midiPlayer_load_result = pthread_create(&midiPlayer_load_thread, NULL, loadMidiFile, mp_args);
    fprintf(stderr, "MidiPlayer Load Thread created\n");
    fprintf(stderr, "MidiPlayer Load Thread result: %d\n", midiPlayer_load_result);
    if (midiPlayer_load_result != 0)
    {
        free(mp_args);
        exit(1);
    }

    struct loop_args *loop_args = malloc(sizeof(struct loop_args));
    loop_args->TinySoundFont = TinySoundFont;
    loop_args->sample_rate = sample_rate;

    loopTSF_result = pthread_create(&loopTSF_thread, NULL, loop_tsf, loop_args);
    fprintf(stderr, "Loop TSF Thread created\n");
    fprintf(stderr, "Loop TSF Thread result: %d\n", loopTSF_result);
    if (loopTSF_result != 0)
    {
        free(loop_args);
        exit(1);
    }

    pthread_join(midiPlayer_load_thread, NULL);
    pthread_join(loopTSF_thread, NULL);

    tsf_close(TinySoundFont);
    free(loop_args);
    free(mp_args);

    return NULL;
}