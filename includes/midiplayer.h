#ifndef MIDIPLAYER_H
#define MIDIPLAYER_H

#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>

#include "midi_header.h"
#include "midi_data.h"

void playMidiFile(MIDIFile *midiFile);
void loadMidiFile(char *filename);

#endif