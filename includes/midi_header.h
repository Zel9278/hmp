#ifndef MIDI_HEADER_H
#define MIDI_HEADER_H

#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>

typedef struct {
    char chunkID[4];
    uint32_t chunkSize;
    uint16_t formatType;
    uint16_t numberOfTracks;
    uint16_t timeDivision;
} MidiHeader;

typedef struct {
    char chunkID[4];
    uint32_t chunkSize;
} MidiTrack;

#endif