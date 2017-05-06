# libOPNMIDI
libOPNMIDI is a free MIDI to WAV conversion library with OPN2 emulation

OPNMIDI Library:   Copyright (c) 2017 Vitaly Novichkov <admin@wohlnet.ru>

Library is based on the libADLMIDI, a MIDI playing libtary with OPL3 emulation:

**This library in BETA. Please report me any bugs and imperfections you have found**

# Tested on platforms
* Linux GCC 4.8, 4.9, 5.4 / CLang
* Mac OS X CLang (Xcode 7.x)
* Windows MinGW 4.9.x, 5.2
* Android NDK 12b/13

# Key features
* OPN2 emulation
* Stereo sound
* Number of simulated chips can be specified as 1-100 (maximum channels 600!)
* Pan (binary panning, i.e. left/right side on/off)
* Pitch-bender with adjustable range
* Vibrato that responds to RPN/NRPN parameters
* Sustain enable/disable
* MIDI and RMI file support
* loopStart / loopEnd tag support (Final Fantasy VII)
* Use automatic arpeggio with chords to relieve channel pressure
* Support for multiple concurrent MIDI synthesizers (per-track device/port select FF 09 message), can be used to overcome 16 channel limit

# How to build
You can build shared version and additional tools on the Linux when you will run a "make" command and you will have libopnmidi.so in the "bin" directory.

You also can build library manually:
You need to make in the any IDE a library project and put into it next files
(or include those files into subfolder of your exist project instead if you want to use it statically):

* opnmidi.h    - Library API, use it to control library

* Ym2612_Emu.h  - Yamaha OPN2 Emulation header
* fraction.h    - Fraction number handling
* opnbank.h    - bank structures definition
* opnmidi_private.hpp - header of internal private APIs
* opnmidi_mus2mid.h - MUS2MID converter header
* opnmidi_xmi2mid.h - XMI2MID converter header

* Ym2612_Emu.cpp   - code of Yamaha OPN2 emulator by Stéphane Dallongeville, improved by Shay Green
* opnmidi.cpp   - code of library

* opnmidi_load.cpp	- Source of file loading and parsing processing
* opnmidi_midiplay.cpp	- MIDI event sequencer
* opnmidi_opn2.cpp	- OPN2 chips manager
* opnmidi_private.cpp	- some internal functions sources
* opnmidi_mus2mid.c	- MUS2MID converter source
* opnmidi_xmi2mid.c	- XMI2MID converter source

# Working demos

* {Coming soon}

