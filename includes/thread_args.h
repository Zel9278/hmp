#ifndef THREAD_ARGS_H
#define THREAD_ARGS_H

#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

struct initTSF_args
{
    char *soundfont;
    char *midiFile;
};

struct midiPlayer_args
{
    tsf *TinySoundFont;
    char *midiFile;
};

#endif