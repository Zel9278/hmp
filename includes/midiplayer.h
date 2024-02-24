#ifndef MIDIPLAYER_H
#define MIDIPLAYER_H

#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>

#include "midi_header.h"
#include "midi_data.h"
#include "tsf.h"

void playMidiFile(tsf *tsf, MIDIFile *midiFile);
void loadMidiFile(tsf *tsf, char *filename);

#endif