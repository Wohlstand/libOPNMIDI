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
#ifndef MIDIPLAY_PLAYBACK_H
#define MIDIPLAY_PLAYBACK_H

#include "opnmidi.h"
#include <stdint.h>
#include "../audio/audio.h"
#include <string>



extern int stop;

#if !defined(ADLMIDI_ENABLE_HW_DOS)
const char* audio_format_to_str(int format, int is_msb);
void fillAudioFormat(const AudioOutputSpec &spec);

void applyGain(uint8_t *buffer, size_t bufferSize);
#endif


#if !defined(OUTPUT_WAVE_ONLY) && !defined(ADLMIDI_ENABLE_HW_DOS)
int runAudioLoop(OPN2_MIDIPlayer *myDevice, AudioOutputSpec &spec);
#endif

int runWaveOutLoopLoop(OPN2_MIDIPlayer *myDevice, const std::string &musPath, const AudioOutputSpec &obtained, unsigned sampleRate);


#endif /* MIDIPLAY_PLAYBACK_H */
