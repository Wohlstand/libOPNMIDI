/*
 * OPNMIDI Player is a free MIDI player based on a libOPNMIDI,
 * a Software MIDI synthesizer library with OPN2 and OPNA emulation
 *
 * Original ADLMIDI code: Copyright (c) 2010-2014 Joel Yliluoma <bisqwit@iki.fi>
 * ADLMIDI Library API:   Copyright (c) 2015-2026 Vitaly Novichkov <admin@wohlnet.ru>
 *
 * Library is based on the ADLMIDI, a MIDI player for Linux and Windows with OPL3 emulation:
 * http://iki.fi/bisqwit/source/adlmidi.html
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */


#pragma once
#ifndef OPNMIDI_DEV_SETUP_H
#define OPNMIDI_DEV_SETUP_H

#include <string>
#include <vector>
#include "audio/audio.h"
#include "opnmidi.h"

struct Args
{
    unsigned int sampleRate;

    AudioOutputSpec spec;
    AudioOutputSpec obtained;

    /*
     * Set library options by parsing of command line arguments
     */
#if !defined(OUTPUT_WAVE_ONLY)
    bool recordWave;
#endif
    bool scaleModulators;
    bool fullRangedBrightness;
#if !defined(OUTPUT_WAVE_ONLY)
    int loopEnabled;
#endif
    int  modeEMIDI;
    int autoArpeggioEnabled;
    int chanAlloc;
    bool fullPanEnabled;
    int emulator;
    int volumeModel;
    size_t soloTrack;
    int songNumLoad;
    int chipsCount;

    std::vector<int> muteChannels;

    std::string bankPath;
    std::string musPath;

    Args();

    int parseArgs(int argc, char **argv_arr, bool *quit);
};

void printError(const char *err, const char *what = NULL);

extern OPNMIDI_AudioFormat g_audioFormat;
extern float g_gaining;
extern Args s_devSetup;

#endif
