#ifndef SOUNDFONT_H
#define SOUNDFONT_H

#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>

#include "midi_header.h"
#include "midi_data.h"
#include "tsf.h"

void *initTSF(void *context);

#endif