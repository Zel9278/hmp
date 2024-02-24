#ifndef MIDI_HEADER_H
#define MIDI_HEADER_H
#ifdef __cplusplus
    extern "C" {
#endif

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

#ifdef __cplusplus
    }
#endif
#endif