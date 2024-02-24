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
#include "midiplayer.h"
#define TSF_IMPLEMENTATION
#include "tsf.h"

int main(int argc, char **argv)
{
    char *soundfont = argv[1];
    char *midiFile = argv[2];

    if (argc < 3)
    {
        fprintf(stderr, "Usage: %s soundfont.sf2 midiFile->mid\n", argv[0]);
        return 1;
    }

    // loadMidiFile(midiFile);

    tsf *TinySoundFont = tsf_load_filename(soundfont);

    int sample_rate = 48000; // Sample rate (Hz)
    tsf_set_output(TinySoundFont, TSF_STEREO_INTERLEAVED, sample_rate, 0);

    int buffer_length_ms = 100; // Adjust as needed

    int sample_count = (sample_rate * buffer_length_ms) / 1000;

    float buffer[sample_count * 2]; // stereo
    while (1)
    {
        fprintf(stderr, "Press Enter to start rendering...\n");
        scanf("%*c"); // Read and discard a character (in this case, Enter)

        fprintf(stderr, "Rendering...\n");

        tsf_note_on(TinySoundFont, 0, 60, 1.0f);
        // Render audio frames
        tsf_render_float(TinySoundFont, buffer, sample_count, 0);

        // Write audio frames to stdout
        fwrite(buffer, sizeof(float), sample_count * 2, stdout);

        // Calculate delay based on buffer length and sample rate
        int delay_us = (sample_count * 1000000) / sample_rate;
        usleep(delay_us); // Introduce a delay based on buffer length and sample rate
    }

    return 0;
}
