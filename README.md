# libOPNMIDI
libOPNMIDI is a free MIDI to WAV conversion library with OPN2 (YM2612) emulation

OPNMIDI Library: Copyright (c) 2017-2018 Vitaly Novichkov <admin@wohlnet.ru>

Library is based on core of the [libADLMIDI](https://github.com/Wohlstand/libADLMIDI), a MIDI playing library with OPL3 emulation.

# Tested on platforms
* Linux GCC 4.8, 4.9, 5.4 / CLang
* Mac OS X CLang (Xcode 7.x)
* Windows MinGW 4.9.x, 5.2
* Android NDK 12b/13

# Key features
* OPN2 emulation
* Customizable bank of FM patches (You have to use the [bank editor](https://github.com/Wohlstand/OPN2BankEditor) to create own sound bank)
* Stereo sound
* Number of simulated chips can be specified as 1-100 (maximum channels 600!)
* Pan (binary panning, i.e. left/right side on/off)
* Pitch-bender with adjustable range
* Vibrato that responds to RPN/NRPN parameters
* Sustain enable/disable
* MIDI and RMI file support
* loopStart / loopEnd tag support (Final Fantasy VII)
* 111-th controller based loop start (RPG-Maker)
* Use automatic arpeggio with chords to relieve channel pressure
* Support for multiple concurrent MIDI synthesizers (per-track device/port select FF 09 message), can be used to overcome 16 channel limit
* Partial support for XG standard (having more instruments than in one 128:128 GM set and ability to use multiple channels for percussion purposes)
* CC74 affects a modulator scale

# How to build
To build libOPNMIDI you need to use CMake:

```bash
mkdir build
cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
make
sudo make install
```

## Available CMake options
* **CMAKE_PREFIX_PATH** - destinition folder where libADLMIDI will be installed. On Linux it is /usr/local/ by default.
* **CMAKE_BUILD_TYPE** - Build types: **Debug** or **Release**
* **WITH_MIDIPLAY** - (ON/OFF, default OFF) Build demo MIDI player (Requires SDL2 and also pthread on Windows with MinGW)
* **WITH_EMBEDDED_BANKS** - (ON/OFF, default ON) Enable or disable embedded banks (Original ADLMIDI and older versions of libADLMIDI are had embedded-only banks with no ability to load custom banks in runtime).
* **USE_GENS_EMULATOR** - (ON/OFF, default ON) Use GENS 2.10 emulator instead of Nuked OPN2. This emulator has less accuracy and seems outdated (will be replaced with MAME), but it is well optimized for a work on slow devices such as older computers, embedded or mobile devices.

* **libOPNMIDI_STATIC** - (ON/OFF, default ON) Build static library
* **libOPNMIDI_SHARED** - (ON/OFF, default OFF) Build shared library


## You also can build library manually:
You need to make in the any IDE a library project and put into it next files
(or include those files into subfolder of your exist project instead if you want to use it statically):

### Useful macros
* `OPNMIDI_DISABLE_XMI_SUPPORT` - Disables XMI to MIDI converter
* `OPNMIDI_DISABLE_MUS_SUPPORT` - Disables MUS to MIDI converter
* `OPNMIDI_DISABLE_MIDI_SEQUENCER` - Completely disables built-in MIDI sequencer.
* `OPNMIDI_USE_LEGACY_EMULATOR` - Enables Gens 2.10 YM2612 emulator to be used. Nuked OPN2 is used by default if macro is not defined.

### Public header (include)
* opnmidi.h    - Library API, use it to control library

### Internal code (src)
* fraction.hpp  - Fraction number handling
* opnbank.h    - bank structures definition
* opnmidi_private.hpp - header of internal private APIs

* opnmidi.cpp   - code of library

* opnmidi_load.cpp	- Source of file loading and parsing processing
* opnmidi_midiplay.cpp	- MIDI event sequencer
* opnmidi_opn2.cpp	- OPN2 chips manager
* opnmidi_private.cpp	- some internal functions sources

* `chips/opn_chip_base.h`   - Header of base class over all emulation cores
* `chips/opn_chip_base.cpp` - Code of base class over all emulation cores

* chips/gens_opn2.h  - Header of emulator frontent over Gens 2.10 emulator
* chips/gens_opn2.cpp  - Code of emulator frontent over Gens 2.10 emulator
* chips/gens/Ym2612_ChipEmu.h  - Gens 2.10 OPN2 Emulation header
* chips/gens/Ym2612_ChipEmu.cpp   - Code of Gens 2.10 OPN2 emulator by Stéphane Dallongeville, improved by Shay Green

* chips/mame_opn2.h  - Header of emulator frontent over MAME YM2612 emulator
* chips/mame_opn2.cpp  - Code of emulator frontent over MAME YM2612 emulator
* chips/mame/mame_ym2612fm.h  - MAME YM2612 Emulation header
* chips/mame/mame_ym2612fm.cpp   - Code of MAME YM2612 emulator by Stéphane Dallongeville, improved by Shay Green

* chips/nuked_opn2.h  - Header of emulator frontent over Nuked OPN2 emulator
* chips/nuked_opn2.cpp  - Code of emulator frontent over Nuked OPN2 emulator
* chips/nuked/ym3438.h  - Nuked OPN2 Emulation header
* chips/nuked/ym3438.cpp   - Code of Nuked OPN2 emulator by Stéphane Dallongeville, improved by Shay Green

#### MUS2MIDI converter
To remove MUS support, define `OPNMIDI_DISABLE_MUS_SUPPORT` macro and remove those files:
* opnmidi_mus2mid.h - MUS2MID converter header
* opnmidi_mus2mid.c	- MUS2MID converter source

#### XMI2MIDI converter
To remove XMI support, define `OPNMIDI_DISABLE_XMI_SUPPORT` macro and remove those files:
* opnmidi_xmi2mid.h - XMI2MID converter header
* opnmidi_xmi2mid.c	- XMI2MID converter source

**Important**: Please use GENS emulator for on mobile or any non-power devices because it requires very small CPU power. Nuked OPN2 emulator is very accurate (compared to real OPN2 chip), however, it requires a VERY POWERFUL device even for a single chip emulation and is a high probability that your device will lag and playback will be dirty and choppy.

# Working demos

* [PGE MusPlay for Win32](http://wohlsoft.ru/docs/_laboratory/_Builds/win32/bin-w32/_packed/pge-musplay-dev-win32.zip) and [Win64](http://wohlsoft.ru/docs/_laboratory/_Builds/win32/bin-w64/_packed/pge-musplay-dev-win64.zip) (also available for other platforms as part of [PGE Project](https://github.com/WohlSoft/PGE-Project)) - a little music player which uses SDL Mixer X library (fork of the SDL Mixer 2.0) which has embedded libOPNMIDI to play MIDI files independently from operating system's settings and drivers. <br>(source code of player can be find [here](https://github.com/WohlSoft/PGE-Project/tree/master/MusicPlayer) and source code of SDL Mixer X [here](https://github.com/WohlSoft/SDL-Mixer-X/))
* [OPNMIDI Player for Android](https://github.com/Wohlstand/OPNMIDI-Player-Java/) - a little MIDI-player for Android which uses libOPNMIDI to play MIDI files and provides flexible GUI with ability to change bank, flags, number of emulated chips, etc.

# Changelog
## 1.2.0   dev
 * Added ability to disable MUS and XMI converters
 * Added ability to disable embedded MIDI sequencer to use library as RealTime synthesizer only or use any custom MIDI sequencer plugins.
 * Fixed blank instruments fallback in multi-bank support. When using non-zero bank, if instrument is blank, then, instrument will be taken from a root (I.e. zero bank).
 * Added support for real-time switching the emulator
 * Added support for MAME YM2612 Emulator

## 1.1.0   2018-01-21
* First stable release
* Contains all features are made in libADLMIDI 1.3.1

## 1.0.0-beta   2017-05-06
* First experimental release

