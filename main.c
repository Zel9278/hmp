/*
The MIT License (MIT)

Copyright (C) 2024 Zel9278
tsf.h By Copyright (C) 2017-2023 Bernhard Schelling (Based on SFZero, Copyright (C) 2012 Steve Folta, https://github.com/stevefolta/SFZero)

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
of the Software, and to permit persons to whom the Software is furnished to do
so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#include <stdio.h>
#include <math.h>

#include "midi_header.h"
#include "midi_data.h"
#define TSF_IMPLEMENTATION
#include "tsf.h"

uint16_t swap_uint16( uint16_t val ) 
{
    return (val << 8) | (val >> 8 );
}

uint32_t swap_uint32( uint32_t val )
{
    val = ((val << 8) & 0xFF00FF00 ) | ((val >> 8) & 0xFF00FF ); 
    return (val << 16) | (val >> 16);
}

long GetNs()
{
    return 0; // TODO
}

void playMidiFile(MIDIFile *midiFile)
{
    long sleepold = 0;
    long sleepdelta = 0;

    long ticker;

    long tempo = 500000;
    long tickMult = fmax((tempo * 10) / midiFile->Division, 1);

    for (int c = 0; c < midiFile->TrackCount; c++)
    {
        uint64_t byte1;
        uint16_t nval = 0;

        do
        {
            byte1 = midiFile->Data[midiFile->TrackOffsets[c]++];
            nval = (uint16_t)((nval << 7) + (byte1 & 0x7F));

            printf("Byte1: %d\n", byte1);
            printf("Nval: %d\n", nval);
        } while (byte1 >= 0x80);

        midiFile->Tracks[c].Tick += nval;
    }

    ticker = GetNs();

    midiFile->LastTime = ticker;

    int activeTracks = midiFile->TrackCount;

    printf("Playing MIDI file...\n");
    printf("Tempo: %ld\n", tempo);
    printf("TickMult: %ld\n", tickMult);
    printf("Running: %d\n", midiFile->Running);

    while (midiFile->Running)
    {
        uint32_t minTick = UINT32_MAX;

        for (int i = 0; i < midiFile->TrackCount; i++)
        {
            while (midiFile->Tracks[i].Tick <= midiFile->CurrentTick && !midiFile->Tracks[i].Ended)
            {
                uint64_t byteValue = midiFile->Data[midiFile->TrackOffsets[i]];
                //printf("ByteValue: %ld\n", byteValue);

                if ((byteValue & 0x80) != 0)
                {
                    midiFile->Tracks[i].RunningStatus = byteValue;
                    midiFile->TrackOffsets[i]++;
                }

                if (midiFile->Tracks[i].RunningStatus < 0xc0 || (0xe0 <= midiFile->Tracks[i].RunningStatus && midiFile->Tracks[i].RunningStatus < 0xf0))
                {   
/*                     OnMIDIEvent?.Invoke(this, new MIDIEventArgs(
                        midiFile->Tracks[i].RunningStatus,
                        midiFile->Data[midiFile->TrackOffsets[i]],
                        midiFile->Data[midiFile->TrackOffsets[i] + 1]
                    )); */

                    switch(midiFile->Tracks[i].RunningStatus)
                    {
                        case 0x80:
                            int status = midiFile->Tracks[i].RunningStatus;
                            int note = midiFile->Data[midiFile->TrackOffsets[i]++];
                            int velocity = midiFile->Data[midiFile->TrackOffsets[i]++];

                            printf("Note off: %d %d %d\n", status, note, velocity);
                            break;
                        case 0x90:
                            status = midiFile->Tracks[i].RunningStatus;
                            note = midiFile->Data[midiFile->TrackOffsets[i]++];
                            velocity = midiFile->Data[midiFile->TrackOffsets[i]++];

                            printf("Note on: %d %d %d\n", status, note, velocity);
                            break;
                    }

                    midiFile->TrackOffsets[i] += 2;
                }
                else if (midiFile->Tracks[i].RunningStatus < 0xe0)
                {
/*                     OnMIDIEvent?.Invoke(this, new MIDIEventArgs(
                        midiFile->Tracks[i].RunningStatus,
                        midiFile->Data[midiFile->TrackOffsets[i]++]
                    )); */
                }
                else if ((midiFile->Tracks[i].RunningStatus & 0xf0) != 0)
                {
                    uint32_t temp2 = 0;
                    if (midiFile->Tracks[i].RunningStatus != 0xf0)
                        temp2 = midiFile->Data[midiFile->TrackOffsets[i]++];

                    long lmsglen = 0;
                    uint32_t byte1;

                    do
                    {
                        byte1 = midiFile->Data[midiFile->TrackOffsets[i]++];
                        lmsglen = (lmsglen << 7) + (byte1 & 0x7F);
                    } while (byte1 >= 0x80);

                    if ((midiFile->Tracks[i].RunningStatus & 0xFF) == 0xF0)
                    {
                        //add longmsg later
                    }
                    else
                    {
                        if (temp2 == 0x51)
                        {
                            tempo =
                                (midiFile->Data[midiFile->TrackOffsets[i]] << 16) |
                                (midiFile->Data[midiFile->TrackOffsets[i] + 1] << 8) |
                                midiFile->Data[midiFile->TrackOffsets[i] + 2];
                            tickMult = fmax((tempo * 10) / midiFile->Division, 1);
                        }
                        else if (temp2 == 0x2F)
                        {
                            midiFile->Tracks[i].Ended = true;
                            activeTracks--;
                        }
                    }
                    midiFile->TrackOffsets[i] += (int)lmsglen;
                }

                long nval2 = 0;
                uint32_t byte2;

                int length = sizeof(midiFile->Data) / sizeof(midiFile->Data[0]);

                do
                {
                    byte2 = (uint32_t)(midiFile->TrackOffsets[i] < length ? midiFile->Data[midiFile->TrackOffsets[i]] : 0);

                    if (midiFile->TrackOffsets[i] < length)
                    {
                        nval2 = (nval2 << 7) + (byte2 & 0x7F);
                        midiFile->TrackOffsets[i]++;
                    }
                } while (byte2 >= 0x80 && midiFile->TrackOffsets[i] < length);


                midiFile->Tracks[i].Tick += (uint32_t)nval2;
            }

            if (!midiFile->Tracks[i].Ended && midiFile->Tracks[i].Tick < minTick)
                minTick = midiFile->Tracks[i].Tick;
        }

        if (activeTracks == 0)
        {
            //Stop();
            //UnLoad();
            break;
        }

        uint32_t deltaTick = minTick - midiFile->CurrentTick;

        midiFile->CurrentTick += deltaTick;

        ticker = GetNs();

        long temp = ticker - midiFile->LastTime;
        midiFile->LastTime = ticker;

        temp -= sleepold;

        sleepold = (long)round((double)(deltaTick * tickMult));
        sleepdelta += temp;
        if (sleepdelta > 0) temp = sleepold - sleepdelta;
        else temp = sleepold;

        long del = temp / 10;
        if (temp > 0)
        {
            //SleepNanos(del * 1000);
        }
    }
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

    midiFile.Data = malloc(sizeof(uint64_t) * dataSize);
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

        //printf("Track %d size: %d bytes\n", i, bytesRead);
        printf("Total read: %d bytes\n", offset);
    }

    playMidiFile(&midiFile);

    fclose(file);

    printf("MIDI file loaded\n");
}

int main(int argc, char** argv)
{
    char *soundfont = argv[1];
    char *midiFile = argv[2];

    if (argc < 3) {
        printf("Usage: %s soundfont.sf2 midiFile->mid\n", argv[0]);
        return 1;
    }

    loadMidiFile(midiFile);

    /*     int time = 22050;

    tsf* TinySoundFont = tsf_load_filename(soundfont);
    tsf_set_output(TinySoundFont, TSF_STEREO_INTERLEAVED, 48000, 0);
    tsf_note_on(TinySoundFont, 0, 60, 1.0f);
    float HalfSecond[time];
    tsf_render_float(TinySoundFont, HalfSecond, time/2, 0);

    fwrite(HalfSecond, sizeof(float), time, stdout); */

    return 0;
}
