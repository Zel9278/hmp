#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>

#include "midiplayer.h"

uint16_t swap_uint16(uint16_t val)
{
    return (val << 8) | (val >> 8);
}

uint32_t swap_uint32(uint32_t val)
{
    val = ((val << 8) & 0xFF00FF00) | ((val >> 8) & 0xFF00FF);
    return (val << 16) | (val >> 16);
}

void sleep_nanos(long nanos)
{
    struct timespec sleepTime;
    sleepTime.tv_sec = 0;
    sleepTime.tv_nsec = nanos;

    nanosleep(&sleepTime, NULL);
}

long get_ns()
{
    clock_t ticks = clock();
    return (long)ticks;
}

void playMidiFile(MIDIFile *midiFile)
{
    uint32_t sleep_old = 0;
    uint32_t sleep_delta = 0;

    uint32_t ticker = 0;

    uint32_t tempo = 500000;
    uint32_t tick_mult = fmaxf((tempo * 10) / midiFile->Division, 1);

    for (int c = 0; c < midiFile->TrackCount; c++)
    {
        int nval = 0;
        uint8_t byte1;

        do
        {
            byte1 = midiFile->Data[midiFile->TrackOffsets[c]];
            midiFile->TrackOffsets[c] += 1;
            nval = (nval << 7) + (byte1 & 0x7F);
        } while (byte1 >= 0x80);

        midiFile->Tracks[c].Tick += nval;
    }

    ticker = get_ns();

    midiFile->LastTime = ticker;

    int activeTracks = midiFile->TrackCount;

    while (midiFile->Running)
    {
        uint32_t min_tick = 4294967295;

        for (int i = 0; i < midiFile->TrackCount; i++)
        {
            while (midiFile->Tracks[i].Tick <= midiFile->CurrentTick && !midiFile->Tracks[i].Ended)
            {
                uint8_t byteValue = midiFile->Data[midiFile->TrackOffsets[i]];
                if (byteValue != 0)
                {
                    printf("Byte: %d\n", byteValue);
                }

                if ((byteValue & 0x80) != 0)
                {
                    midiFile->Tracks[i].RunningStatus = byteValue;
                    midiFile->TrackOffsets[i]++;
                }

                if (midiFile->Tracks[i].RunningStatus < 0xc0 || (0xe0 <= midiFile->Tracks[i].RunningStatus && midiFile->Tracks[i].RunningStatus < 0xf0))
                {
                    int statusByte = midiFile->Tracks[i].RunningStatus;
                    int dataByte1 = midiFile->Data[midiFile->TrackOffsets[i]];
                    int dataByte2 = midiFile->Data[midiFile->TrackOffsets[i] + 1];

                    switch (statusByte)
                    {
                    case 0x80:
                        int channel = statusByte & 0x0F;
                        int note = dataByte1;

                        printf("Note off - Channel: %d, Note: %d", channel, note);
                        break;
                    case 0x90:
                        break;
                    }

                    midiFile->TrackOffsets[i] += 2;
                }
                else if (midiFile->Tracks[i].RunningStatus < 0xe0)
                {
                    int statusByte = midiFile->Tracks[i].RunningStatus;
                    int dataByte1 = midiFile->Data[midiFile->TrackOffsets[i]++];

                    switch (statusByte)
                    {
                    case 0x80:
                        int channel = statusByte & 0x0F;
                        int note = dataByte1;

                        printf("Note off - Channel: %d, Note: %d", channel, note);
                        break;
                    case 0x90:
                        break;
                    }
                }
                else if ((midiFile->Tracks[i].RunningStatus & 0xF0) != 0)
                {
                    uint8_t temp2 = 0;
                    if (midiFile->Tracks[i].RunningStatus != 0xf0)
                        temp2 = midiFile->Data[midiFile->TrackOffsets[i]++];

                    long lmsglen = 0;
                    uint64_t byte1;

                    do
                    {
                        byte1 = midiFile->Data[midiFile->TrackOffsets[i]++];
                        lmsglen = (lmsglen << 7) + (byte1 & 0x7F);
                    } while (byte1 >= 0x80);

                    if ((midiFile->Tracks[i].RunningStatus & 0xFF) != 0xF0)
                    {
                        if (temp2 == 0x51)
                        {
                            tempo =
                                (midiFile->Data[midiFile->TrackOffsets[i]] << 16) |
                                (midiFile->Data[midiFile->TrackOffsets[i] + 1] << 8) |
                                midiFile->Data[midiFile->TrackOffsets[i] + 2];
                            tick_mult = fmaxf((tempo * 10) / midiFile->Division, 1);
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
                uint8_t byte2;

                do
                {
                    byte2 = (midiFile->TrackOffsets[i] < midiFile->DataLength ? midiFile->Data[midiFile->TrackOffsets[i]] : 0);
                    if (midiFile->TrackOffsets[i] < midiFile->DataLength)
                    {
                        nval2 = (nval2 << 7) + (byte2 & 0x7F);
                        midiFile->TrackOffsets[i]++;
                    }
                } while (byte2 >= 0x80 && midiFile->TrackOffsets[i] < midiFile->DataLength);

                midiFile->Tracks[i].Tick += (uint32_t)nval2;
            }

            if (!midiFile->Tracks[i].Ended && midiFile->Tracks[i].Tick < min_tick)
                min_tick = midiFile->Tracks[i].Tick;
        }

        if (activeTracks == 0)
        {
            break;
        }

        uint32_t delta_tick = min_tick - midiFile->CurrentTick;

        midiFile->CurrentTick += delta_tick;

        ticker = get_ns();

        long temp = ticker - midiFile->LastTime;
        midiFile->LastTime = ticker;

        temp -= sleep_old;

        sleep_old = (long)round((double)(delta_tick * tick_mult));
        sleep_delta += temp;
        if (sleep_delta > 0)
            temp = sleep_old - sleep_delta;
        else
            temp = sleep_old;

        long del = temp / 10;
        if (temp > 0)
        {
            sleep_nanos(del * 1000);
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
    midiFile.DataLength = 0;
    midiFile.TrackOffsets = NULL;
    midiFile.Tracks = NULL;

    FILE *file = fopen(filename, "rb");
    if (!file)
    {
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

    if (strcmp(header.chunkID, "MThd"))
    {
        printf("Error: %s is not a valid MIDI file\n", filename);
    }

    header.chunkSize = swap_uint32(header.chunkSize);
    header.formatType = swap_uint16(header.formatType);
    header.numberOfTracks = swap_uint16(header.numberOfTracks);
    header.timeDivision = swap_uint16(header.timeDivision);

    if (header.chunkSize != 6)
    {
        printf("Error: %s is not a valid Header Length\n", filename);
    }

    if (header.formatType != 1)
    {
        printf("Error: %s is not a valid Format Type\n", filename);
    }

    if (header.timeDivision > 0x8000)
    {
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
    midiFile.DataLength = dataSize;

    if (!midiFile.Data)
    {
        printf("Error: could not allocate memory for MIDI data\n");
    }

    int trackOffsets[midiFile.TrackCount];
    midiFile.TrackOffsets = (int *)malloc(sizeof(int) * midiFile.TrackCount);

    Track tracks[midiFile.TrackCount];
    midiFile.Tracks = (Track *)malloc(sizeof(Track) * midiFile.TrackCount);

    midiFile.Running = true;

    fseek(file, 14, SEEK_SET);

    int offset = 0;

    for (int i = 0; i < header.numberOfTracks; i++)
    {
        MidiTrack track;
        fread(&track, sizeof(MidiTrack), 1, file);

        printf("-- Track %d\n", i);

        if (strcmp(track.chunkID, "MTrk"))
        {
            printf("Error: Track %d is not a valid Track\n", i);
        }

        track.chunkSize = swap_uint32(track.chunkSize);

        midiFile.Tracks[i].RunningStatus = 0;
        midiFile.Tracks[i].Tick = 0;
        midiFile.Tracks[i].Ended = false;

        midiFile.TrackOffsets[i] = offset;
        printf("Offset: %d\n", midiFile.TrackOffsets[i]);

        int bytesRead = fread(&midiFile.Data[offset], 1, track.chunkSize, file);

        offset += bytesRead;

        if (bytesRead != track.chunkSize)
        {
            printf("Error: could not read track data\n");
        }

        printf("Track %d size: %d bytes\n", i, bytesRead);
        printf("Total read: %d bytes\n", offset);
    }

    playMidiFile(&midiFile);

    free(midiFile.TrackOffsets);
    free(midiFile.Tracks);
    free(midiFile.Data);

    fclose(file);

    printf("MIDI file loaded\n");
}