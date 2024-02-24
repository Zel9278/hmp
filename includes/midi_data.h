#ifndef MIDI_DATA_H
#define MIDI_DATA_H

#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>

typedef struct {
    int RunningStatus;
    uint64_t Tick;
    bool Ended;
} Track;

typedef struct {
    int TrackCount;
    int Division;
    uint16_t CurrentTick;
    uint64_t *Data;
    int DataLength;
    int *TrackOffsets;
    Track *Tracks;
    bool Running;
    long LastTime;
} MIDIFile;

#endif