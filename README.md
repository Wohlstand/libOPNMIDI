# libOPNMIDI
libOPNMIDI is a free MIDI to WAV conversion library with OPL3 emulation

OPNMIDI Library:   Copyright (c) 2017 Vitaly Novichkov <admin@wohlnet.ru>

Library is based on the libADLMIDI, a MIDI playing libtary with OPL3 emulation:

**This library in WIP**

# Tested on platforms
* Linux GCC 4.8, 4.9, 5.4 / CLang
* Mac OS X CLang (Xcode 7.x)
* Windows MinGW 4.9.x, 5.2
* Android NDK 12b/13

# Key features
* OPN2 emulation
* Stereo sound
* Number of simulated soundcards can be specified as 1-100 (maximum channels 1800!)
* Pan (binary panning, i.e. left/right side on/off)
* Pitch-bender with adjustable range
* Vibrato that responds to RPN/NRPN parameters
* Sustain enable/disable
* MIDI and RMI file support
* loopStart / loopEnd tag support (Final Fantasy VII)
* Use automatic arpeggio with chords to relieve channel pressure
* Support for multiple concurrent MIDI synthesizers (per-track device/port select FF 09 message), can be used to overcome 16 channel limit

# Working demos

* {Coming soon}
