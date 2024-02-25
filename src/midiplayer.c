#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include <pthread.h>

#include "midiplayer.h"
#include "thread_args.h"

uint16_t swap_uint16(uint16_t val)
{
    return (val << 8) | (val >> 8);
}

uint32_t swap_uint32(uint32_t val)
{
    val = ((val << 8) & 0xFF00FF00) | ((val >> 8) & 0xFF00FF);
    return (val << 16) | (val >> 16);
}

void delayExecution(long nanoseconds)
{
    struct timespec ts;
    ts.tv_sec = 0;
    ts.tv_nsec = nanoseconds;
    nanosleep(&ts, NULL);
}

long long get_ns() {
    struct timespec ts;

    // Get the current time using CLOCK_MONOTONIC_RAW (or similar clock)
    clock_gettime(CLOCK_MONOTONIC_RAW, &ts);

    // Convert the time to nanoseconds
    long long nanoseconds = ts.tv_sec * 1000000000LL + ts.tv_nsec;

    return nanoseconds;
}

void sleepNanos(long long nanos) {
    long long start = get_ns();
    while (1) {
        // Get the current time
        long long now = get_ns();
        long long elapsed = now - start;
        // Check if the elapsed time is greater than or equal to the specified nanoseconds
        if (elapsed >= nanos) {
            return;
        }
    }
}

void *playMidiFile(void *contents)
{
    struct midiPlayer_play_args *args = (struct midiPlayer_play_args *)contents;
    tsf *tsf = args->TinySoundFont;
    MIDIFile *midiFile = args->midiFile;

    long long sleep_old = 0;
    long long sleep_delta = 0;
    long long lastTime = 0;

    long long ticker = 0;

    uint32_t tempo = 500000;
    double tick_mult = fmaxf((tempo * 10) / (double)midiFile->Division, 1);

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

    lastTime = ticker;

    int activeTracks = midiFile->TrackCount;

    while (midiFile->Running)
    {
        uint64_t min_tick = UINT64_MAX;

        for (int i = 0; i < midiFile->TrackCount; i++)
        {
            Track *track = &(midiFile->Tracks[i]);
            while (track->Tick <= midiFile->CurrentTick && !track->Ended)
            {
                uint8_t byteValue = *track->head;

                if (byteValue & 0x80)
                {
                    track->head++;
                    track->RunningStatus = byteValue;
                }

                if (track->RunningStatus < 0xc0 || (0xe0 <= track->RunningStatus && track->RunningStatus < 0xf0))
                {
                    int statusByte = track->RunningStatus;
                    int dataByte1 = *track->head;
                    int dataByte2 = *(track->head + 1);

                    switch (statusByte & 0xF0)
                    {
                    case 0x80:
                    {

                        int channel = statusByte & 0x0F;
                        int note = dataByte1;

                        // fprintf(stderr, "Note off - Channel: %d, Note: %d\n", channel, note);
                        tsf_note_off(tsf, 0, note);
                    }
                    break;
                    case 0x90:
                    {

                        int channel = statusByte & 0x0F;
                        int note = dataByte1;
                        int velocity = dataByte2;

                        tsf_note_on(tsf, 0, note, velocity / 127.0f);
                        // fprintf(stderr, "Note on - Channel: %d, Note: %d, Velocity: %d\n", channel, note, velocity);
                    }
                    break;
                    }

                    track->head += 2;
                }
                else if (track->RunningStatus < 0xe0)
                {
                    int statusByte = track->RunningStatus;
                    char dataByte1 = track->head++;

                    switch (statusByte)
                    {
                        // later
                    }
                }
                else if (track->RunningStatus & 0xF0)
                {
                    uint8_t temp2 = 0;
                    if (track->RunningStatus != 0xf0)
                        temp2 = *track->head++;

                    uint32_t lmsglen = 0;
                    uint8_t byte1;

                    do
                    {
                        byte1 = *track->head++;
                        lmsglen = (lmsglen << 7) + (byte1 & 0x7F);
                    } while (byte1 >= 0x80);

                    if ((track->RunningStatus & 0xFF) != 0xF0)
                    {
                        if (temp2 == 0x51)
                        {
                            tempo = *track->head << 16 | *(track->head + 1) << 8 | *(track->head + 2);
                            tick_mult = fmaxf((tempo * 10) / (float)midiFile->Division, 1);
                            // fprintf(stderr, "Tempo change: %d, BPM: %.2f\n", tempo, 60000000.0 / (float)tempo);
                        }
                        else if (temp2 == 0x2F)
                        {
                            track->Ended = true;
                            activeTracks--;
                        }
                    }

                    track->head += (int)lmsglen;
                }

                uint32_t nval2 = 0;
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
                    byte2 = *track->head++;
                    nval2 = (nval2 << 7) + (byte2 & 0x7F);
                } while (byte2 >= 0x80);
                track->Tick += nval2;
            }

            if (!track->Ended && track->Tick < min_tick)
                min_tick = track->Tick;
        }

        if (!activeTracks)
        {
            break;
        }

        uint32_t delta_tick = min_tick - midiFile->CurrentTick;
        midiFile->CurrentTick += delta_tick;

        //fprintf(stderr, "delta tick: %ld\n", delta_tick);


        ticker = get_ns();

        // fprintf(stderr, "ticker: %ld nanoseconds\n", ticker);

        //playback will drift over time
        long long temp = ticker - lastTime;
        fprintf(stderr, "temp: %ld\n", temp);
        // fprintf(stderr, "temp: %ld nanoseconds\n", ticker);
        lastTime = ticker;

        temp -= sleep_old;
        sleep_old = delta_tick * tick_mult;
        sleep_delta += temp;
        //fprintf(stderr, "sleepdelta: %ld nanoseconds\n", sleep_old);

        if (sleep_delta > 0)
            temp = sleep_old - sleep_delta;
        else
            temp = sleep_old;

        uint64_t del = temp * 100;
        // if (temp > 0)
        //{
        //  usleep(del * 100);
        //delayExecution(del);

        usleep(sleep_old / 10);

        //fprintf(stderr, "Sleeping for %ld nanoseconds\n", del);
        //}

        // unsigned int deltaTick = min_tick - midiFile->CurrentTick;
        // midiFile->CurrentTick += deltaTick;

        // struct timespec ticker;
        // clock_gettime(CLOCK_MONOTONIC, &ticker);
        // double lastTime = ticker.tv_sec + ticker.tv_nsec / 1.0e9;

        // double temp = ((double)clock() - lastTime) * CLOCKS_PER_SEC;

        // lastTime = clock();
        // temp -= sleepOld;
        // sleepOld = deltaTick * tickMultiplier;
        // clock_gettime(CLOCK_MONOTONIC, &ticker);
        // double currentTime = ticker.tv_sec + ticker.tv_nsec / 1.0e9;
        // double sleepDelta = currentTime - lastTime;

        // if (sleepDelta > 0)
        // {
        //     temp = sleepOld - sleepDelta;
        // }
        // else
        // {
        //     temp = sleepOld;
        // }

        // if (temp > 0)
        // {
        //     delayExecution((long)(temp * 1.0e9)); // converting seconds to nanoseconds
        // }
    }

    return NULL;
}

void *loadMidiFile(void *context)
{
    struct midiPlayer_load_args *args = (struct midiPlayer_load_args *)context;
    tsf *tsf = args->TinySoundFont;
    char *filename = args->midiFile;

    pthread_t midiPlayer_play_thread;
    int midiPlayer_play_result = 0;

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
            continue;
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

    struct midiPlayer_play_args *mp_args = malloc(sizeof(struct midiPlayer_play_args));
    mp_args->TinySoundFont = tsf;
    mp_args->midiFile = &midiFile;

    midiPlayer_play_result = pthread_create(&midiPlayer_play_thread, NULL, playMidiFile, mp_args);
    fprintf(stderr, "MidiPlayer Play Thread created\n");
    fprintf(stderr, "MidiPlayer Play Thread result: %d\n", midiPlayer_play_result);
    if (midiPlayer_play_result != 0)
    {
        free(mp_args);
        exit(1);
    }

    pthread_join(midiPlayer_play_thread, NULL);

    free(mp_args);

    free(midiFile.Tracks);
    free(midiFile.Data);

    fclose(file);

    fprintf(stderr, "MIDI file loaded\n");

    return NULL;
}