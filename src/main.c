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
#include <stdlib.h>
#include <math.h>
#include <pthread.h>
#include <unistd.h>

#include "midi_header.h"
#include "midi_data.h"
#include "soundfont.h"
#include "midiplayer.h"
#include "thread_args.h"

int main(int argc, char **argv)
{
    char *soundfont = argv[1];
    char *midiFile = argv[2];

    if (argc < 3)
    {
        fprintf(stderr, "Usage: %s soundfont.sf2 midiFile->mid\n", argv[0]);
        return 1;
    }

    pthread_t tsf_thread;
    int tsf_result = 0;

    struct initTSF_args *itsf_args = malloc(sizeof(struct initTSF_args));
    itsf_args->soundfont = soundfont;
    itsf_args->midiFile = midiFile;

    tsf_result = pthread_create(&tsf_thread, NULL, initTSF, itsf_args);
    fprintf(stderr, "Soundfont Thread created\n");
    fprintf(stderr, "Soundfont Thread result: %d\n", tsf_result);
    if (tsf_result != 0) {
        free(itsf_args);
        exit(1);
    }

    pthread_join(tsf_thread, NULL);
    free(itsf_args);

    return 0;
}