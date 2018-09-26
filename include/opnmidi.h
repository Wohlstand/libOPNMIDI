/*
 * libOPNMIDI is a free MIDI to WAV conversion library with OPN2 (YM2612) emulation
 *
 * MIDI parser and player (Original code from ADLMIDI): Copyright (c) 2010-2014 Joel Yliluoma <bisqwit@iki.fi>
 * OPNMIDI Library and YM2612 support:   Copyright (c) 2017-2018 Vitaly Novichkov <admin@wohlnet.ru>
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

#ifndef OPNMIDI_H
#define OPNMIDI_H

#ifdef __cplusplus
extern "C" {
#endif

#define OPNMIDI_VERSION_MAJOR       1
#define OPNMIDI_VERSION_MINOR       4
#define OPNMIDI_VERSION_PATCHLEVEL  0

#define OPNMIDI_TOSTR_I(s) #s
#define OPNMIDI_TOSTR(s) OPNMIDI_TOSTR_I(s)
#define OPNMIDI_VERSION \
        OPNMIDI_TOSTR(OPNMIDI_VERSION_MAJOR) "." \
        OPNMIDI_TOSTR(OPNMIDI_VERSION_MINOR) "." \
        OPNMIDI_TOSTR(OPNMIDI_VERSION_PATCHLEVEL)

#include <stddef.h>

#if defined(__STDC_VERSION__) && (__STDC_VERSION__ >= 199901L)
#include <stdint.h>
typedef uint8_t         OPN2_UInt8;
typedef uint16_t        OPN2_UInt16;
typedef int8_t          OPN2_SInt8;
typedef int16_t         OPN2_SInt16;
#else
typedef unsigned char   OPN2_UInt8;
typedef unsigned short  OPN2_UInt16;
typedef char            OPN2_SInt8;
typedef short           OPN2_SInt16;
#endif

#ifdef OPNMIDI_BUILD
#   ifndef OPNMIDI_DECLSPEC
#       if defined (_WIN32) && defined(OPNMIDI_BUILD_DLL)
#           define OPNMIDI_DECLSPEC __declspec(dllexport)
#       else
#           define OPNMIDI_DECLSPEC
#       endif
#   endif
#else
#   define OPNMIDI_DECLSPEC
#endif


enum OPNMIDI_VolumeModels
{
    OPNMIDI_VolumeModel_AUTO = 0,
    OPNMIDI_VolumeModel_Generic,
    OPNMIDI_VolumeModel_NativeOPN2,
    OPNMIDI_VolumeModel_DMX,
    OPNMIDI_VolumeModel_APOGEE,
    OPNMIDI_VolumeModel_9X
};

enum OPNMIDI_SampleType
{
    OPNMIDI_SampleType_S16 = 0,  /* signed PCM 16-bit */
    OPNMIDI_SampleType_S8,       /* signed PCM 8-bit */
    OPNMIDI_SampleType_F32,      /* float 32-bit */
    OPNMIDI_SampleType_F64,      /* float 64-bit */
    OPNMIDI_SampleType_S24,      /* signed PCM 24-bit */
    OPNMIDI_SampleType_S32,      /* signed PCM 32-bit */
    OPNMIDI_SampleType_U8,       /* unsigned PCM 8-bit */
    OPNMIDI_SampleType_U16,      /* unsigned PCM 16-bit */
    OPNMIDI_SampleType_U24,      /* unsigned PCM 24-bit */
    OPNMIDI_SampleType_U32,      /* unsigned PCM 32-bit */
    OPNMIDI_SampleType_Count,
};

struct OPNMIDI_AudioFormat
{
    enum OPNMIDI_SampleType type;  /* type of sample */
    unsigned containerSize;        /* size in bytes of the storage type */
    unsigned sampleOffset;         /* distance in bytes between consecutive samples */
};

struct OPN2_MIDIPlayer
{
    void *opn2_midiPlayer;
};

/* DEPRECATED */
#define opn2_setNumCards opn2_setNumChips

/* Sets number of emulated sound cards (from 1 to 100). Emulation of multiple sound cards exchanges polyphony limits*/
extern OPNMIDI_DECLSPEC int  opn2_setNumChips(struct OPN2_MIDIPlayer *device, int numCards);

/* Get current number of emulated chips */
extern OPNMIDI_DECLSPEC int  opn2_getNumChips(struct OPN2_MIDIPlayer *device);

/**
 * @brief Reference to dynamic bank
 */
typedef struct OPN2_Bank
{
    void *pointer[3];
} OPN2_Bank;

/**
 * @brief Identifier of dynamic bank
 */
typedef struct OPN2_BankId
{
    /*! 0 if bank is melodic set, or 1 if bank is a percussion set */
    OPN2_UInt8 percussive;
    /*! Assign to MSB bank number */
    OPN2_UInt8 msb;
    /*! Assign to LSB bank number */
    OPN2_UInt8 lsb;
} OPN2_BankId;

/**
 * @brief Flags for dynamic bank access
 */
enum OPN2_BankAccessFlags
{
    /*! create bank, allocating memory as needed */
    OPNMIDI_Bank_Create = 1,
    /*! create bank, never allocating memory */
    OPNMIDI_Bank_CreateRt = 1|2
};

typedef struct OPN2_Instrument OPN2_Instrument;




/* ======== Setup ======== */

#ifdef OPNMIDI_UNSTABLE_API

/**
 * @brief Preallocates a minimum number of bank slots. Returns the actual capacity
 * @param device Instance of the library
 * @param banks Count of bank slots to pre-allocate.
 * @return actual capacity of reserved bank slots.
 */
extern OPNMIDI_DECLSPEC int opn2_reserveBanks(struct OPN2_MIDIPlayer *device, unsigned banks);
/**
 * @brief Gets the bank designated by the identifier, optionally creating if it does not exist
 * @param device Instance of the library
 * @param id Identifier of dynamic bank
 * @param flags Flags for dynamic bank access (OPN2_BankAccessFlags)
 * @param bank Reference to dynamic bank
 * @return 0 on success, <0 when any error has occurred
 */
extern OPNMIDI_DECLSPEC int opn2_getBank(struct OPN2_MIDIPlayer *device, const OPN2_BankId *id, int flags, OPN2_Bank *bank);
/**
 * @brief Gets the identifier of a bank
 * @param device Instance of the library
 * @param bank Reference to dynamic bank.
 * @param id Identifier of dynamic bank
 * @return 0 on success, <0 when any error has occurred
 */
extern OPNMIDI_DECLSPEC int opn2_getBankId(struct OPN2_MIDIPlayer *device, const OPN2_Bank *bank, OPN2_BankId *id);
/**
 * @brief Removes a bank
 * @param device Instance of the library
 * @param bank Reference to dynamic bank
 * @return 0 on success, <0 when any error has occurred
 */
extern OPNMIDI_DECLSPEC int opn2_removeBank(struct OPN2_MIDIPlayer *device, OPN2_Bank *bank);
/**
 * @brief Gets the first bank
 * @param device Instance of the library
 * @param bank Reference to dynamic bank
 * @return 0 on success, <0 when any error has occurred
 */
extern OPNMIDI_DECLSPEC int opn2_getFirstBank(struct OPN2_MIDIPlayer *device, OPN2_Bank *bank);
/**
 * @brief Iterates to the next bank
 * @param device Instance of the library
 * @param bank Reference to dynamic bank
 * @return 0 on success, <0 when any error has occurred or end has been reached.
 */
extern OPNMIDI_DECLSPEC int opn2_getNextBank(struct OPN2_MIDIPlayer *device, OPN2_Bank *bank);
/**
 * @brief Gets the nth intrument in the bank [0..127]
 * @param device Instance of the library
 * @param bank Reference to dynamic bank
 * @param index Index of the instrument
 * @param ins Instrument entry
 * @return 0 on success, <0 when any error has occurred
 */
extern OPNMIDI_DECLSPEC int opn2_getInstrument(struct OPN2_MIDIPlayer *device, const OPN2_Bank *bank, unsigned index, OPN2_Instrument *ins);
/**
 * @brief Sets the nth intrument in the bank [0..127]
 * @param device Instance of the library
 * @param bank Reference to dynamic bank
 * @param index Index of the instrument
 * @param ins Instrument structure pointer
 * @return 0 on success, <0 when any error has occurred
 *
 * This function allows to override an instrument on the fly
 */
extern OPNMIDI_DECLSPEC int opn2_setInstrument(struct OPN2_MIDIPlayer *device, OPN2_Bank *bank, unsigned index, const OPN2_Instrument *ins);

#endif  /* OPNMIDI_UNSTABLE_API */



/*Override Enable(1) or Disable(0) LFO. -1 - use bank default state*/
extern OPNMIDI_DECLSPEC void opn2_setLfoEnabled(struct OPN2_MIDIPlayer *device, int lfoEnable);

/*Get the LFO state*/
extern OPNMIDI_DECLSPEC int opn2_getLfoEnabled(struct OPN2_MIDIPlayer *device);

/*Override LFO frequency. -1 - use bank default state*/
extern OPNMIDI_DECLSPEC void opn2_setLfoFrequency(struct OPN2_MIDIPlayer *device, int lfoFrequency);

/*Get the LFO frequency*/
extern OPNMIDI_DECLSPEC int opn2_getLfoFrequency(struct OPN2_MIDIPlayer *device);

/*Enable or disable Enables scaling of modulator volumes*/
extern OPNMIDI_DECLSPEC void opn2_setScaleModulators(struct OPN2_MIDIPlayer *device, int smod);

/*Enable(1) or Disable(0) full-range brightness (MIDI CC74 used in XG music to filter result sounding) scaling.
    By default, brightness affects sound between 0 and 64.
    When this option is enabled, the range will use a full range from 0 up to 127.
*/
extern OPNMIDI_DECLSPEC void opn2_setFullRangeBrightness(struct OPN2_MIDIPlayer *device, int fr_brightness);

/*Enable or disable built-in loop (built-in loop supports 'loopStart' and 'loopEnd' tags to loop specific part)*/
extern OPNMIDI_DECLSPEC void opn2_setLoopEnabled(struct OPN2_MIDIPlayer *device, int loopEn);

/* !!!DEPRECATED!!! */
extern OPNMIDI_DECLSPEC void opn2_setLogarithmicVolumes(struct OPN2_MIDIPlayer *device, int logvol);

/*Set different volume range model */
extern OPNMIDI_DECLSPEC void opn2_setVolumeRangeModel(struct OPN2_MIDIPlayer *device, int volumeModel);

/*Get the volume range model */
extern OPNMIDI_DECLSPEC int opn2_getVolumeRangeModel(struct OPN2_MIDIPlayer *device);

/*Load WOPN bank file from File System. Is recommended to call adl_reset() to apply changes to already-loaded file player or real-time.*/
extern OPNMIDI_DECLSPEC int opn2_openBankFile(struct OPN2_MIDIPlayer *device, const char *filePath);

/*Load WOPN bank file from memory data*/
extern OPNMIDI_DECLSPEC int opn2_openBankData(struct OPN2_MIDIPlayer *device, const void *mem, long size);


/* DEPRECATED */
extern OPNMIDI_DECLSPEC const char *opn2_emulatorName();

/*Returns chip emulator name string*/
extern OPNMIDI_DECLSPEC const char *opn2_chipEmulatorName(struct OPN2_MIDIPlayer *device);

enum Opn2_Emulator
{
    OPNMIDI_EMU_MAME = 0,
    OPNMIDI_EMU_NUKED,
    OPNMIDI_EMU_GENS,
    OPNMIDI_EMU_GX,
    OPNMIDI_EMU_end
};

/* Switch the emulation core */
extern OPNMIDI_DECLSPEC int opn2_switchEmulator(struct OPN2_MIDIPlayer *device, int emulator);

typedef struct {
    OPN2_UInt16 major;
    OPN2_UInt16 minor;
    OPN2_UInt16 patch;
} OPN2_Version;

/*Run emulator with PCM rate to reduce CPU usage on slow devices. May decrease sounding accuracy.*/
extern OPNMIDI_DECLSPEC int opn2_setRunAtPcmRate(struct OPN2_MIDIPlayer *device, int enabled);

/*Returns string which contains a version number*/
extern OPNMIDI_DECLSPEC const char *opn2_linkedLibraryVersion();

/*Returns structure which contains a version number of library */
extern OPNMIDI_DECLSPEC const OPN2_Version *opn2_linkedVersion();

/*Returns string which contains last error message*/
extern OPNMIDI_DECLSPEC const char *opn2_errorString();

/*Returns string which contains last error message on specific device*/
extern OPNMIDI_DECLSPEC const char *opn2_errorInfo(struct OPN2_MIDIPlayer *device);

/*Initialize ADLMIDI Player device*/
extern OPNMIDI_DECLSPEC struct OPN2_MIDIPlayer *opn2_init(long sample_rate);

/*Set 4-bit device identifier*/
extern OPNMIDI_DECLSPEC int opn2_setDeviceIdentifier(struct OPN2_MIDIPlayer *device, unsigned id);

/*Load MIDI file from File System*/
extern OPNMIDI_DECLSPEC int opn2_openFile(struct OPN2_MIDIPlayer *device, const char *filePath);

/*Load MIDI file from memory data*/
extern OPNMIDI_DECLSPEC int opn2_openData(struct OPN2_MIDIPlayer *device, const void *mem, unsigned long size);

/*Resets MIDI player*/
extern OPNMIDI_DECLSPEC void opn2_reset(struct OPN2_MIDIPlayer *device);

/*Get total time length of current song*/
extern OPNMIDI_DECLSPEC double opn2_totalTimeLength(struct OPN2_MIDIPlayer *device);

/*Get loop start time if presented. -1 means MIDI file has no loop points */
extern OPNMIDI_DECLSPEC double opn2_loopStartTime(struct OPN2_MIDIPlayer *device);

/*Get loop end time if presented. -1 means MIDI file has no loop points */
extern OPNMIDI_DECLSPEC double opn2_loopEndTime(struct OPN2_MIDIPlayer *device);

/*Get current time position in seconds*/
extern OPNMIDI_DECLSPEC double opn2_positionTell(struct OPN2_MIDIPlayer *device);

/*Jump to absolute time position in seconds*/
extern OPNMIDI_DECLSPEC void opn2_positionSeek(struct OPN2_MIDIPlayer *device, double seconds);

/*Reset MIDI track position to begin */
extern OPNMIDI_DECLSPEC void opn2_positionRewind(struct OPN2_MIDIPlayer *device);

/*Set tempo multiplier: 1.0 - original tempo, >1 - play faster, <1 - play slower */
extern OPNMIDI_DECLSPEC void opn2_setTempo(struct OPN2_MIDIPlayer *device, double tempo);

/*Get a textual description of the chip channel state. For display only.*/
extern OPNMIDI_DECLSPEC int opn2_describeChannels(struct OPN2_MIDIPlayer *device, char *text, char *attr, size_t size);

/*Close and delete OPNMIDI device*/
extern OPNMIDI_DECLSPEC void opn2_close(struct OPN2_MIDIPlayer *device);



/**META**/

/*Returns string which contains a music title*/
extern OPNMIDI_DECLSPEC const char *opn2_metaMusicTitle(struct OPN2_MIDIPlayer *device);

/*Returns string which contains a copyright string*/
extern OPNMIDI_DECLSPEC const char *opn2_metaMusicCopyright(struct OPN2_MIDIPlayer *device);

/*Returns count of available track titles: NOTE: there are CAN'T be associated with channel in any of event or note hooks */
extern OPNMIDI_DECLSPEC size_t opn2_metaTrackTitleCount(struct OPN2_MIDIPlayer *device);

/*Get track title by index*/
extern OPNMIDI_DECLSPEC const char *opn2_metaTrackTitle(struct OPN2_MIDIPlayer *device, size_t index);

struct Opn2_MarkerEntry
{
    const char      *label;
    double          pos_time;
    unsigned long   pos_ticks;
};

/*Returns count of available markers*/
extern OPNMIDI_DECLSPEC size_t opn2_metaMarkerCount(struct OPN2_MIDIPlayer *device);

/*Returns the marker entry*/
extern OPNMIDI_DECLSPEC struct Opn2_MarkerEntry opn2_metaMarker(struct OPN2_MIDIPlayer *device, size_t index);




/*Take a sample buffer and iterate MIDI timers */
extern OPNMIDI_DECLSPEC int  opn2_play(struct OPN2_MIDIPlayer *device, int sampleCount, short *out);

/*Take a sample buffer and iterate MIDI timers */
extern OPNMIDI_DECLSPEC int  opn2_playFormat(struct OPN2_MIDIPlayer *device, int sampleCount, OPN2_UInt8 *left, OPN2_UInt8 *right, const struct OPNMIDI_AudioFormat *format);

/*Generate audio output from chip emulators without iteration of MIDI timers.*/
extern OPNMIDI_DECLSPEC int  opn2_generate(struct OPN2_MIDIPlayer *device, int sampleCount, short *out);

/*Generate audio output from chip emulators without iteration of MIDI timers.*/
extern OPNMIDI_DECLSPEC int  opn2_generateFormat(struct OPN2_MIDIPlayer *device, int sampleCount, OPN2_UInt8 *left, OPN2_UInt8 *right, const struct OPNMIDI_AudioFormat *format);

/**
 * @brief Periodic tick handler.
 * @param device
 * @param seconds seconds since last call
 * @param granularity don't expect intervals smaller than this, in seconds
 * @return desired number of seconds until next call
 *
 * Use it for Hardware OPL3 mode or when you want to process events differently from opn2_play() function.
 * DON'T USE IT TOGETHER WITH opn2_play()!!!
 */
extern OPNMIDI_DECLSPEC double opn2_tickEvents(struct OPN2_MIDIPlayer *device, double seconds, double granuality);

/*Returns 1 if music position has reached end*/
extern OPNMIDI_DECLSPEC int opn2_atEnd(struct OPN2_MIDIPlayer *device);

/**
 * @brief Returns the number of tracks of the current sequence
 * @param device Instance of the library
 * @return Count of tracks in the current sequence
 */
extern OPNMIDI_DECLSPEC size_t opn2_trackCount(struct OPN2_MIDIPlayer *device);

/**
 * @brief Track options
 */
enum OPNMIDI_TrackOptions
{
    /*! Enabled track */
    OPNMIDI_TrackOption_On   = 1,
    /*! Disabled track */
    OPNMIDI_TrackOption_Off  = 2,
    /*! Solo track */
    OPNMIDI_TrackOption_Solo = 3,
};

/**
 * @brief Sets options on a track of the current sequence
 * @param device Instance of the library
 * @param trackNumber Identifier of the designated track.
 * @return 0 on success, <0 when any error has occurred
 */
extern OPNMIDI_DECLSPEC int opn2_setTrackOptions(struct OPN2_MIDIPlayer *device, size_t trackNumber, unsigned trackOptions);

/**RealTime**/

/*Force Off all notes on all channels*/
extern OPNMIDI_DECLSPEC void opn2_panic(struct OPN2_MIDIPlayer *device);

/*Reset states of all controllers on all MIDI channels*/
extern OPNMIDI_DECLSPEC void opn2_rt_resetState(struct OPN2_MIDIPlayer *device);

/*Turn specific MIDI note ON*/
extern OPNMIDI_DECLSPEC int opn2_rt_noteOn(struct OPN2_MIDIPlayer *device, OPN2_UInt8 channel, OPN2_UInt8 note, OPN2_UInt8 velocity);

/*Turn specific MIDI note OFF*/
extern OPNMIDI_DECLSPEC void opn2_rt_noteOff(struct OPN2_MIDIPlayer *device, OPN2_UInt8 channel, OPN2_UInt8 note);

/*Set note after-touch*/
extern OPNMIDI_DECLSPEC void opn2_rt_noteAfterTouch(struct OPN2_MIDIPlayer *device, OPN2_UInt8 channel, OPN2_UInt8 note, OPN2_UInt8 atVal);
/*Set channel after-touch*/
extern OPNMIDI_DECLSPEC void opn2_rt_channelAfterTouch(struct OPN2_MIDIPlayer *device, OPN2_UInt8 channel, OPN2_UInt8 atVal);

/*Apply controller change*/
extern OPNMIDI_DECLSPEC void opn2_rt_controllerChange(struct OPN2_MIDIPlayer *device, OPN2_UInt8 channel, OPN2_UInt8 type, OPN2_UInt8 value);

/*Apply patch change*/
extern OPNMIDI_DECLSPEC void opn2_rt_patchChange(struct OPN2_MIDIPlayer *device, OPN2_UInt8 channel, OPN2_UInt8 patch);

/*Apply pitch bend change*/
extern OPNMIDI_DECLSPEC void opn2_rt_pitchBend(struct OPN2_MIDIPlayer *device, OPN2_UInt8 channel, OPN2_UInt16 pitch);
/*Apply pitch bend change*/
extern OPNMIDI_DECLSPEC void opn2_rt_pitchBendML(struct OPN2_MIDIPlayer *device, OPN2_UInt8 channel, OPN2_UInt8 msb, OPN2_UInt8 lsb);

/*Change LSB of the bank*/
extern OPNMIDI_DECLSPEC void opn2_rt_bankChangeLSB(struct OPN2_MIDIPlayer *device, OPN2_UInt8 channel, OPN2_UInt8 lsb);
/*Change MSB of the bank*/
extern OPNMIDI_DECLSPEC void opn2_rt_bankChangeMSB(struct OPN2_MIDIPlayer *device, OPN2_UInt8 channel, OPN2_UInt8 msb);
/*Change bank by absolute signed value*/
extern OPNMIDI_DECLSPEC void opn2_rt_bankChange(struct OPN2_MIDIPlayer *device, OPN2_UInt8 channel, OPN2_SInt16 bank);

/*Perform a system exclusive message*/
extern OPNMIDI_DECLSPEC int opn2_rt_systemExclusive(struct OPN2_MIDIPlayer *device, const OPN2_UInt8 *msg, size_t size);

/**Hooks**/

typedef void (*OPN2_RawEventHook)(void *userdata, OPN2_UInt8 type, OPN2_UInt8 subtype, OPN2_UInt8 channel, const OPN2_UInt8 *data, size_t len);
typedef void (*OPN2_NoteHook)(void *userdata, int adlchn, int note, int ins, int pressure, double bend);
typedef void (*OPN2_DebugMessageHook)(void *userdata, const char *fmt, ...);

/* Set raw MIDI event hook */
extern OPNMIDI_DECLSPEC void opn2_setRawEventHook(struct OPN2_MIDIPlayer *device, OPN2_RawEventHook rawEventHook, void *userData);

/* Set note hook */
extern OPNMIDI_DECLSPEC void opn2_setNoteHook(struct OPN2_MIDIPlayer *device, OPN2_NoteHook noteHook, void *userData);

/* Set debug message hook */
extern OPNMIDI_DECLSPEC void opn2_setDebugMessageHook(struct OPN2_MIDIPlayer *device, OPN2_DebugMessageHook debugMessageHook, void *userData);




/* ======== Instrument structures ======== */

/**
 * @brief Version of the instrument data format
 */
enum
{
    OPNMIDI_InstrumentVersion = 0
};

/**
 * @brief Instrument flags
 */
typedef enum OPN2_InstrumentFlags
{
    OPNMIDI_Ins_Pseudo8op  = 0x01, /*Reserved for future use, not implemented yet*/
    OPNMIDI_Ins_IsBlank    = 0x02
} OPN2_InstrumentFlags;

/**
 * @brief Operator structure, part of Instrument structure
 */
typedef struct OPN2_Operator
{
    /* Detune and frequency multiplication register data */
    OPN2_UInt8 dtfm_30;
    /* Total level register data */
    OPN2_UInt8 level_40;
    /* Rate scale and attack register data */
    OPN2_UInt8 rsatk_50;
    /* Amplitude modulation enable and Decay-1 register data */
    OPN2_UInt8 amdecay1_60;
    /* Decay-2 register data */
    OPN2_UInt8 decay2_70;
    /* Sustain and Release register data */
    OPN2_UInt8 susrel_80;
    /* SSG-EG register data */
    OPN2_UInt8 ssgeg_90;
} OPN2_Operator;

/**
 * @brief Instrument structure
 */
typedef struct OPN2_Instrument
{
    /*! Version of the instrument object */
    int version;
    /* MIDI note key (half-tone) offset for an instrument (or a first voice in pseudo-4-op mode) */
    OPN2_SInt16 note_offset;
    /* Reserved */
    OPN2_SInt8  midi_velocity_offset;
    /* Percussion MIDI base tone number at which this drum will be played */
    OPN2_UInt8 percussion_key_number;
    /* Instrument flags */
    OPN2_UInt8 inst_flags;
    /* Feedback and Algorithm register data */
    OPN2_UInt8 fbalg;
    /* LFO Sensitivity register data */
    OPN2_UInt8 lfosens;
    /* Operators register data */
    OPN2_Operator operators[4];
    /* Millisecond delay of sounding while key is on */
    OPN2_UInt16 delay_on_ms;
    /* Millisecond delay of sounding after key off */
    OPN2_UInt16 delay_off_ms;
} OPN2_Instrument;

#ifdef __cplusplus
}
#endif

#endif /* OPNMIDI_H */
