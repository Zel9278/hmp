#ifndef MIDI_DATA_H
#define MIDI_DATA_H

#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

typedef struct {
    unsigned char *head;
    int RunningStatus;
    uint64_t Tick;
    bool Ended;
} Track;

typedef struct {
    uint8_t Status;
    uint8_t Data1;
    uint8_t Data2;
} MidiEvent;

typedef struct {
    int TrackCount;
    int Division;
    uint64_t CurrentTick;
    unsigned char *Data;
    int DataLength;
    Track *Tracks;
    bool Running;
    long long LastTime;
} MIDIFile;

#endif