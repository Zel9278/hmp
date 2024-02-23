#include <stdio.h>

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

    // MThdか確認
    if (strcmp(header.chunkID, "MThd")) {
        printf("Error: %s is not a valid MIDI file\n", filename);
    }

    // エンディアン変換
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

    // ヘッダ情報の表示
    printf("FormatType: %d\n", header.formatType);
    printf("TrackSize: %d\n", header.numberOfTracks);
    printf("Division: %d\n", header.timeDivision);

    unsigned long dataSize = fileSize - ((unsigned long)header.numberOfTracks * 8 + 14);
    printf("- Allocating %lu bytes for data...\n", dataSize);

    // MIDIFileに書き込む
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

    // データ部分のサイズを取得
    fseek(file, 14, SEEK_SET);

    int offset = 0;

    for (int i = 0; i < header.numberOfTracks; i++) {
        MidiTrack track;
        fread(&track, sizeof(MidiTrack), 1, file);

        printf("-- Track %d\n", i);
        printf("ChunkID: %s\n", track.chunkID);

        // MTrkか確認
        if (strcmp(track.chunkID, "MTrk")) {
            printf("Error: %s is not a valid Track\n", filename);
        }

        // エンディアン変換
        track.chunkSize = swap_uint32(track.chunkSize);

        printf("ChunkSize: %d\n", track.chunkSize);

        midiFile.Tracks[i].RunningStatus = 0;
        midiFile.Tracks[i].Tick = 0;
        midiFile.Tracks[i].Ended = false;

        midiFile.TrackOffsets[i] = offset;

        int bytesRead = fread(&midiFile.Data[offset], 1, track.chunkSize, file);

        offset += bytesRead;

        if (bytesRead != track.chunkSize) {
            printf("Error: could not read track data\n");
        }

        printf("Track %d size: %d bytes\n", i + 1, bytesRead);
        printf("Total read: %d bytes\n", offset);
    }

    free(midiFile.Data);

    fclose(file);

    printf("MIDI file loaded\n");
}

int main(int argc, char** argv)
{
    char *soundfont = argv[1];
    char *midiFile = argv[2];

    if (argc < 3) {
        printf("Usage: %s soundfont.sf2 midiFile.mid\n", argv[0]);
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
