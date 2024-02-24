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

void *initTSF(void *context)
{
    struct initTSF_args *args = (struct initTSF_args *)context;
    char *soundfont = args->soundfont;
    char *midiFile = args->midiFile;

    pthread_t midiPlayer_thread;
    int midiPlayer_result = 0;

    tsf *TinySoundFont = tsf_load_filename(soundfont);

    int sample_rate = 48000;
    tsf_set_output(TinySoundFont, TSF_STEREO_INTERLEAVED, sample_rate, 0);
    tsf_set_max_voices(TinySoundFont, 1000);

    int buffer_length_ms = 100;

    int sample_count = (sample_rate * buffer_length_ms) / 1000;

    struct midiPlayer_args *mp_args = malloc(sizeof(struct midiPlayer_args));
    mp_args->TinySoundFont = TinySoundFont;
    mp_args->midiFile = midiFile;

    midiPlayer_result = pthread_create(&midiPlayer_thread, NULL, loadMidiFile, mp_args);
    printf("MidiPlayer Thread created\n");
    printf("MidiPlayer Thread result: %d\n", midiPlayer_result);
    if (midiPlayer_result != 0) {
        tsf_close(TinySoundFont);
        free(mp_args);
        exit(1);
    }

    pthread_join(midiPlayer_thread, NULL);

    float buffer[sample_count * 2];
    while (1)
    {
        tsf_render_float(TinySoundFont, buffer, sample_count, 0);

        fwrite(buffer, sizeof(float), sample_count * 2, stdout);

        int delay_us = (sample_count * 1000000) / sample_rate;
        usleep(delay_us);
    }

    tsf_close(TinySoundFont);
    free(mp_args);

    return NULL;
}