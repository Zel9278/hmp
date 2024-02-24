#ifndef THREAD_ARGS_H
#define THREAD_ARGS_H

#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

#include "midi_header.h"
#include "tsf.h"

struct initTSF_args
{
    char *soundfont;
    char *midiFile;
};

struct midiPlayer_load_args
{
    tsf *TinySoundFont;
    char *midiFile;
};

struct midiPlayer_play_args
{
    tsf *TinySoundFont;
    MIDIFile *midiFile;
};

#endif