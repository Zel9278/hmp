#ifndef MIDI_DATA_H
#define MIDI_DATA_H
#ifdef __cplusplus
    extern "C" {
#endif

#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>

typedef struct {
    int RunningStatus;
    int Tick;
    bool Ended;
} Track;

typedef struct {
    int TrackCount;
    int Division;
    uint16_t CurrentTick;
    unsigned char *Data;
    int *TrackOffsets;
    Track *Tracks;
    bool Running;
    long Lasttime;
} MIDIFile;

#ifdef __cplusplus
    }
#endif
#endif