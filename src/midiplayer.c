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
    struct timespec ts;
    clock_gettime(0, &ts);
    return (unsigned long long)ts.tv_sec * 1000000000 + ts.tv_nsec;
}

void playMidiFile(tsf *tsf, MIDIFile *midiFile)
{
    double sleep_old = 0;
    double sleep_delta = 0;

    uint32_t ticker = 0;

    uint32_t tempo = 500000;
    float tick_mult = fmaxf((tempo * 10) / (float)midiFile->Division, 1);

    for (int c = 0; c < midiFile->TrackCount; c++)
    {
        int nval = 0;
        uint8_t byte1;

        do
        {
            byte1 = *(midiFile->Tracks[c].head)++;
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
                uint8_t byteValue = *(midiFile->Tracks[i]).head;

                if ((byteValue & 0x80) != 0)
                {
                    midiFile->Tracks[i].head++;
                    midiFile->Tracks[i].RunningStatus = byteValue;
                }

                if (midiFile->Tracks[i].RunningStatus < 0xc0 || (0xe0 <= midiFile->Tracks[i].RunningStatus && midiFile->Tracks[i].RunningStatus < 0xf0))
                {
                    int statusByte = midiFile->Tracks[i].RunningStatus;
                    int dataByte1 = *midiFile->Tracks[i].head;
                    int dataByte2 = *(midiFile->Tracks[i].head + 1);

                    switch (statusByte)
                    {
                    case 0x80:
                    {

                        int channel = statusByte & 0x0F;
                        int note = dataByte1;

                        fprintf(stderr, "Note off - Channel: %d, Note: %d\n", channel, note);
                        tsf_note_off(tsf, 0, note);
                    }
                    break;
                    case 0x90:
                    {

                        int channel = statusByte & 0x0F;
                        int note = dataByte1;
                        int velocity = dataByte2;

                        tsf_note_on(tsf, 0, note, velocity / 127.0f);
                        fprintf(stderr, "Note on - Channel: %d, Note: %d, Velocity: %d\n", channel, note, velocity);
                    }
                    break;
                    }

                    midiFile->Tracks[i].head += 2;
                }
                else if (midiFile->Tracks[i].RunningStatus < 0xe0)
                {
                    int statusByte = midiFile->Tracks[i].RunningStatus;
                    int dataByte1 = *(midiFile->Tracks[i]).head++;

                    switch (statusByte)
                    {
                        // later
                    }
                }
                else if ((midiFile->Tracks[i].RunningStatus & 0xF0) != 0)
                {
                    uint8_t temp2 = 0;
                    if (midiFile->Tracks[i].RunningStatus != 0xf0)
                        temp2 = *(midiFile->Tracks[i]).head++;

                    long lmsglen = 0;
                    uint64_t byte1;

                    do
                    {
                        byte1 = *(midiFile->Tracks[i]).head++;
                        lmsglen = (lmsglen << 7) + (byte1 & 0x7F);
                    } while (byte1 >= 0x80);

                    if ((midiFile->Tracks[i].RunningStatus & 0xFF) != 0xF0)
                    {
                        if (temp2 == 0x51)
                        {
                            tempo =
                                (*(midiFile->Tracks[i]).head << 16) |
                                (*((midiFile->Tracks[i]).head + 1) << 8) |
                                *((midiFile->Tracks[i]).head + 2);
                            tick_mult = fmaxf((tempo * 10) / (float)midiFile->Division, 1);
                            fprintf(stderr, "Tempo change: %d, BPM: %.2f\n", tempo, 60000000.0 / (float)tempo);
                        }
                        else if (temp2 == 0x2F)
                        {
                            midiFile->Tracks[i].Ended = true;
                            activeTracks--;
                        }
                    }

                    midiFile->Tracks[i].head += (int)lmsglen;
                }

                long nval2 = 0;
                uint8_t byte2;

                /*
                do
                {
                    byte2 = (((intptr_t)(midiFile->Tracks[i].head - midiFile->Data[0])) ? midiFile->Data[*(midiFile->Tracks[i]).head] : 0);
                    if (((intptr_t)(midiFile->Tracks[i].head - midiFile->Data[0])) < midiFile->DataLength)
                    {
                        nval2 = (nval2 << 7) + (byte2 & 0x7F);
                        midiFile->Tracks[i].head++;
                    }
                } while (byte2 >= 0x80 && ((intptr_t)(midiFile->Tracks[i].head - midiFile->Data[0])) < midiFile->DataLength);
                */
                do
                {
                    byte2 = *(midiFile->Tracks[i].head)++;
                    nval2 = (nval2 << 7) + (byte2 & 0x7F);
                } while (byte2 >= 0x80);
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
        float temp = ticker - midiFile->LastTime;
        midiFile->LastTime = ticker;
        
        temp -= sleep_old;
        sleep_old = round((double)(delta_tick * tick_mult));

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

void loadMidiFile(tsf *tsf, char *filename)
{
    MIDIFile midiFile;
    midiFile.TrackCount = 0;
    midiFile.Division = 0;
    midiFile.CurrentTick = 0;
    midiFile.Data = NULL;
    midiFile.DataLength = 0;
    midiFile.Tracks = NULL;

    FILE *file = fopen(filename, "rb");
    if (!file)
    {
        fprintf(stderr, "Error: could not open file %s\n", filename);
    }

    unsigned long fileSize;
    fseek(file, 0, SEEK_END);
    fileSize = ftell(file);
    fseek(file, 0, SEEK_SET);

    MidiHeader header;
    fread(&header, sizeof(MidiHeader), 1, file);

    fprintf(stderr, "-- Header\n");
    fprintf(stderr, "ChunkID: %s\n", header.chunkID);

    if (strcmp(header.chunkID, "MThd"))
    {
        fprintf(stderr, "Error: %s is not a valid MIDI file\n", filename);
    }

    header.chunkSize = swap_uint32(header.chunkSize);
    header.formatType = swap_uint16(header.formatType);
    header.numberOfTracks = swap_uint16(header.numberOfTracks);
    header.timeDivision = swap_uint16(header.timeDivision);

    if (header.chunkSize != 6)
    {
        fprintf(stderr, "Error: %s is not a valid Header Length\n", filename);
    }

    if (header.formatType != 1)
    {
        fprintf(stderr, "Error: %s is not a valid Format Type\n", filename);
    }

    if (header.timeDivision > 0x8000)
    {
        fprintf(stderr, "Error: SMTPE time code is not supported\n");
    }

    fprintf(stderr, "FormatType: %d\n", header.formatType);
    fprintf(stderr, "TrackSize: %d\n", header.numberOfTracks);
    fprintf(stderr, "Division: %d\n", header.timeDivision);

    unsigned long dataSize = fileSize - ((unsigned long)header.numberOfTracks * 8 + 14);
    fprintf(stderr, "- Allocating %lu bytes for data...\n", dataSize);

    midiFile.Division = header.timeDivision;
    midiFile.TrackCount = header.numberOfTracks;
    midiFile.CurrentTick = 0;

    midiFile.Data = malloc(sizeof(uint64_t) * dataSize);
    midiFile.DataLength = dataSize;

    if (!midiFile.Data)
    {
        fprintf(stderr, "Error: could not allocate memory for MIDI data\n");
    }

    Track tracks[midiFile.TrackCount];
    midiFile.Tracks = (Track *)malloc(sizeof(Track) * midiFile.TrackCount);

    midiFile.Running = true;

    fseek(file, 14, SEEK_SET);

    int offset = 0;

    for (int i = 0; i < header.numberOfTracks; i++)
    {
        MidiTrack track;
        fread(&track, sizeof(MidiTrack), 1, file);

        fprintf(stderr, "-- Track %d\n", i);
        fprintf(stderr, "ChunkID: %s\n", track.chunkID);

        if (strcmp(track.chunkID, "MTrk"))
        {
            fprintf(stderr, "Error: Track %d is not a valid Track\n", i);
        }

        track.chunkSize = swap_uint32(track.chunkSize);

        midiFile.Tracks[i].head = &midiFile.Data[offset];
        midiFile.Tracks[i].RunningStatus = 0;
        midiFile.Tracks[i].Tick = 0;
        midiFile.Tracks[i].Ended = false;

        int bytesRead = fread(&midiFile.Data[offset], 1, track.chunkSize, file);

        offset += bytesRead;

        if (bytesRead != track.chunkSize)
        {
            fprintf(stderr, "Error: could not read track data\n");
        }

        fprintf(stderr, "Track %d size: %d bytes\n", i, bytesRead);
        fprintf(stderr, "Total read: %d bytes\n", offset);
    }

    playMidiFile(&tsf, &midiFile);

    free(midiFile.Tracks);
    free(midiFile.Data);

    fclose(file);

    fprintf(stderr, "MIDI file loaded\n");
}