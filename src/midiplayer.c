#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "midiplayer.h"

uint16_t swap_uint16( uint16_t val ) 
{
    return (val << 8) | (val >> 8 );
}

uint32_t swap_uint32( uint32_t val )
{
    val = ((val << 8) & 0xFF00FF00 ) | ((val >> 8) & 0xFF00FF ); 
    return (val << 16) | (val >> 16);
}

void playMidiFile(MIDIFile *midiFile)
{
}

void loadMidiFile(char *filename)
{
    MIDIFile midiFile;
    midiFile.TrackCount = 0;
    midiFile.Division = 0;
    midiFile.CurrentTick = 0;
    midiFile.Data = NULL;
    midiFile.TrackOffsets = NULL;
    midiFile.Tracks = NULL;

    FILE *file = fopen(filename, "rb");
    if (!file) {
        printf("Error: could not open file %s\n", filename);
    }

    unsigned long fileSize;
    fseek(file, 0, SEEK_END);
    fileSize = ftell(file);
    fseek(file, 0, SEEK_SET);

    MidiHeader header;
    fread(&header, sizeof(MidiHeader), 1, file);

    printf("-- Header\n");
    printf("ChunkID: %s\n", header.chunkID);

    if (strcmp(header.chunkID, "MThd")) {
        printf("Error: %s is not a valid MIDI file\n", filename);
    }

    header.chunkSize = swap_uint32(header.chunkSize);
    header.formatType = swap_uint16(header.formatType);
    header.numberOfTracks = swap_uint16(header.numberOfTracks);
    header.timeDivision = swap_uint16(header.timeDivision);

    if (header.chunkSize != 6) {
        printf("Error: %s is not a valid Header Length\n", filename);
    }

    if (header.formatType != 1) {
        printf("Error: %s is not a valid Format Type\n", filename);
    }

    if (header.timeDivision > 0x8000) {
        printf("Error: SMTPE time code is not supported\n");
    }

    printf("FormatType: %d\n", header.formatType);
    printf("TrackSize: %d\n", header.numberOfTracks);
    printf("Division: %d\n", header.timeDivision);

    unsigned long dataSize = fileSize - ((unsigned long)header.numberOfTracks * 8 + 14);
    printf("- Allocating %lu bytes for data...\n", dataSize);

    midiFile.Division = header.timeDivision;
    midiFile.TrackCount = header.numberOfTracks;
    midiFile.CurrentTick = 0;

    midiFile.Data = malloc(sizeof(uint32_t) * dataSize);
    if (!midiFile.Data) {
        printf("Error: could not allocate memory for MIDI data\n");
    }

    int trackOffsets[header.numberOfTracks];
    midiFile.TrackOffsets = trackOffsets;

    Track tracks[header.numberOfTracks];
    midiFile.Tracks = tracks;

    midiFile.Running = true;

    fseek(file, 14, SEEK_SET);

    int offset = 0;

    for (int i = 0; i < header.numberOfTracks; i++) {
        MidiTrack track;
        fread(&track, sizeof(MidiTrack), 1, file);

        printf("-- Track %d\n", i);
        //printf("ChunkID: %s\n", track.chunkID);

        if (strcmp(track.chunkID, "MTrk")) {
            printf("Error: Track %d is not a valid Track\n", i);
        }

        track.chunkSize = swap_uint32(track.chunkSize);

        //printf("ChunkSize: %d\n", track.chunkSize);

        midiFile.Tracks[i].RunningStatus = 0;
        midiFile.Tracks[i].Tick = 0;
        midiFile.Tracks[i].Ended = false;

        midiFile.TrackOffsets[i] = offset;

        int bytesRead = fread(&midiFile.Data[offset], 1, track.chunkSize, file);

        offset += bytesRead;

        if (bytesRead != track.chunkSize) {
            printf("Error: could not read track data\n");
        }

        printf("Track %d size: %d bytes\n", i, bytesRead);
        printf("Total read: %d bytes\n", offset);
    }

    playMidiFile(&midiFile);

    free(midiFile.Data);

    fclose(file);

    printf("MIDI file loaded\n");
}