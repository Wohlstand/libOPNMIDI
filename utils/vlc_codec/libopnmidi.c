/*****************************************************************************
 * libopnmidi.c: Software MIDI synthesizer using OPN2 Synth emulation
 *****************************************************************************
 * Copyright © 2017-2025 Vitaly Novichkov
 * $Id$
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
 *****************************************************************************/

#include <vlc_common.h>
#include <vlc_plugin.h>
#include <vlc_codec.h>
#include <vlc_dialog.h>
#include <libvlc_version.h>

#include <unistd.h>

#ifndef N_
#define N_(x) (x)
#endif

#ifndef _
#define _(x) (x)
#endif

#include <opnmidi.h>
#include "gm_opn_bank.h"

#define ENABLE_CODEC_TEXT N_("Enable this synthesizer")
#define ENABLE_CODEC_LONGTEXT N_( \
    "Enable using of this synthesizer, otherwise, other MIDI synthesizers will be used")

#define FMBANK_TEXT N_("Custom bank file")
#define FMBANK_LONGTEXT N_( \
    "Custom bank file (in WOPN format) to use for software synthesis." )

#if 0 /* Old code */
#define CHORUS_TEXT N_("Chorus")

#define GAIN_TEXT N_("Synthesis gain")
#define GAIN_LONGTEXT N_("This gain is applied to synthesis output. " \
    "High values may cause saturation when many notes are played at a time." )

#define POLYPHONY_TEXT N_("Polyphony")
#define POLYPHONY_LONGTEXT N_( \
    "The polyphony defines how many voices can be played at a time. " \
    "Larger values require more processing power.")

#define REVERB_TEXT N_("Reverb")
#endif

#define SAMPLE_RATE_TEXT N_("Sample rate")

#define EMULATED_CHIPS_TEXT N_("Count of emulated chips")
#define EMULATED_CHIPS_LONGTEXT N_( \
    "How many emulated chips will be processed to expand channels limit of a single chip." )

#define EMBEDDED_BANK_ID_TEXT N_("Embedded bank")
#define EMBEDDED_BANK_ID_LONGTEXT N_( \
    "Use one of embedded banks.")

#define FULL_PANNING_TEXT N_("Full panning")
#define FULL_PANNING_LONGTEXT N_( \
    "Enable full-panning stereo support")

#define VOLUME_MODEL_TEXT N_("Volume scaling model")
#define VOLUME_MODEL_LONGTEXT N_( \
    "Declares volume scaling model which will affect volume levels.")

#define CHANNEL_ALLOCATION_TEXT N_("Channel allocation mode")
#define CHANNEL_ALLOCATION_LONGTEXT N_( \
    "Declares the method of chip channel allocation for new notes.")

#define FULL_RANGE_CC74_TEXT N_("Full-range of brightness")
#define FULL_RANGE_CC74_LONGTEXT N_( \
    "Scale range of CC-74 \"Brightness\" with full 0~127 range. By default is only 0~64 affects the sounding.")

#define ENABLE_AUTO_ARPEGGIO_TEXT N_("Enable auto-arpeggio")
#define ENABLE_AUTO_ARPEGGIO_LONGTEXT N_( \
    "Enables an automatical arpeggio to keep chords playing when there is no enough free physical voices of chips.")

static const int volume_models_values[] = { 0, 1, 2, 3, 4, 5 };
static const char * const volume_models_descriptions[] =
{
    N_("Auto (defined by bank)"),
    N_("Generic"),
    N_("OPN2 Native"),
    N_("DMX"),
    N_("Apogee Sound System"),
    N_("Win9x OPL driver"),
    NULL
};

static const int channel_alloc_values[] = { -1, 0, 1, 2 };
static const char * const channel_alloc_descriptions[] =
{
    N_("Auto (defined by bank)"),
    N_("By sounding delays"),
    N_("Released channel of same instrument"),
    N_("Any released channel"),
    NULL
};

#define EMULATOR_TYPE_TEXT N_("OPN2 Emulation core")
#define EMULATOR_TYPE_LINGTEXT N_( \
    "OPN2 Emulator that will be used to generate final sound.")

/*TODO: Turn on fourth emulator when complete experiments with it */
static const int emulator_type_values[] =
{
#ifndef OPNMIDI_DISABLE_MAME_EMULATOR
    (int)OPNMIDI_EMU_MAME,
#endif

#ifndef OPNMIDI_DISABLE_NUKED_EMULATOR
    (int)OPNMIDI_EMU_NUKED_YM3438,
    (int)OPNMIDI_EMU_NUKED_YM2612,
#endif

#ifndef OPNMIDI_DISABLE_GENS_EMULATOR
    (int)OPNMIDI_EMU_GENS,
#endif

#ifdef USE_YMFM_EMULATOR
    (int)OPNMIDI_EMU_YMFM_OPN2,
#endif

#ifndef OPNMIDI_DISABLE_NP2_EMULATOR
    (int)OPNMIDI_EMU_NP2,
#endif

#ifndef OPNMIDI_DISABLE_MAME_2608_EMULATOR
    (int)OPNMIDI_EMU_MAME_2608,
#endif

#ifdef USE_YMFM_EMULATOR
    (int)OPNMIDI_EMU_YMFM_OPNA,
#endif

#ifdef OPNMIDI_ENABLE_OPNA_LLE_EMULATOR
    (int)OPNMIDI_EMU_NUKED_YM2608_LLE,
#endif

#ifdef OPNMIDI_ENABLE_OPN2_LLE_EMULATOR
    (int)OPNMIDI_EMU_NUKED_YM2612_LLE,
    (int)OPNMIDI_EMU_NUKED_YM3438_LLE,
    (int)OPNMIDI_EMU_NUKED_YMF276_LLE,
#endif
};
static const char * const emulator_type_descriptions[] =
{
#ifndef OPNMIDI_DISABLE_MAME_EMULATOR
    N_("MAME YM2612 (OPN2)"),
#endif

#ifndef OPNMIDI_DISABLE_NUKED_EMULATOR
    N_("Nuked OPN2 YM3438"),
    N_("Nuked OPN2 YM2612"),
#endif

#ifndef OPNMIDI_DISABLE_GENS_EMULATOR
    N_("GENS/GS II OPN2"),
#endif

#ifndef OPNMIDI_DISABLE_YMFM_EMULATOR
    N_("YMFM OPN2"),
#endif

#ifndef OPNMIDI_DISABLE_NP2_EMULATOR
    N_("Neko Project II Kai OPNA"),
#endif

#ifndef OPNMIDI_DISABLE_MAME_2608_EMULATOR
    N_("MAME YM2610 (OPNA)"),
#endif

#ifndef OPNMIDI_DISABLE_YMFM_EMULATOR
    N_("YMFM OPNA"),
#endif

#ifdef OPNMIDI_ENABLE_OPNA_LLE_EMULATOR
    N_("Nuked OPNA-YM2608 LLE [!EXTRA HEAVY!]"),
#endif

#ifdef OPNMIDI_ENABLE_OPN2_LLE_EMULATOR
    N_("Nuked OPN2-YM2612 LLE [!EXTRA HEAVY!]"),
    N_("Nuked OPN2-YM3438 LLE [!EXTRA HEAVY!]"),
    N_("Nuked OPN2-YMF276 LLE [!EXTRA HEAVY!]"),
#endif
    NULL
};

static int  Open  (vlc_object_t *);
static void Close (vlc_object_t *);

#define CONFIG_PREFIX "opnmidi-"

vlc_module_begin ()
    set_description (N_("OPNMIDI YM2612 Synth MIDI synthesizer"))
#if (LIBVLC_VERSION_MAJOR >= 3)
    set_capability ("audio decoder", 150)
#else
    set_capability ("decoder", 150)
#endif
    set_shortname (N_("OPNMIDI"))
    set_category (CAT_INPUT)
    set_subcategory (SUBCAT_INPUT_ACODEC)
    set_callbacks (Open, Close)

    add_bool( CONFIG_PREFIX "enable", true, ENABLE_CODEC_TEXT,
                  ENABLE_CODEC_LONGTEXT, false )

    add_loadfile (CONFIG_PREFIX "custombank", "",
                  FMBANK_TEXT, FMBANK_LONGTEXT, false)

    add_integer (CONFIG_PREFIX "volume-model", 0, VOLUME_MODEL_TEXT, VOLUME_MODEL_LONGTEXT, false )
        change_integer_list( volume_models_values, volume_models_descriptions )

    add_integer (CONFIG_PREFIX "channel-allocation", -1, CHANNEL_ALLOCATION_TEXT, CHANNEL_ALLOCATION_LONGTEXT, false )
        change_integer_list( channel_alloc_values, channel_alloc_descriptions )

    add_integer (CONFIG_PREFIX "emulator-type", 0, EMULATOR_TYPE_TEXT, EMULATOR_TYPE_LINGTEXT, false)
        change_integer_list( emulator_type_values, emulator_type_descriptions )

    add_integer (CONFIG_PREFIX "emulated-chips", 6, EMULATED_CHIPS_TEXT, EMULATED_CHIPS_TEXT, false)
        change_integer_range (1, 100)

    add_integer (CONFIG_PREFIX "sample-rate", 44100, SAMPLE_RATE_TEXT, SAMPLE_RATE_TEXT, true)
        change_integer_range (22050, 96000)

    add_bool( CONFIG_PREFIX "full-range-brightness", false, FULL_RANGE_CC74_TEXT,
              FULL_RANGE_CC74_LONGTEXT, true )

    add_bool( CONFIG_PREFIX "enable-auto-arpeggio", true, ENABLE_AUTO_ARPEGGIO_TEXT,
              ENABLE_AUTO_ARPEGGIO_LONGTEXT, true )

    add_bool( CONFIG_PREFIX "full-panning", true, FULL_PANNING_TEXT,
              FULL_PANNING_LONGTEXT, true )

vlc_module_end ()


struct decoder_sys_t
{
    struct OPN2_MIDIPlayer *synth;
    int               sample_rate;
    int               soundfont;
    date_t            end_date;
};

static const struct OPNMIDI_AudioFormat g_output_format =
{
    OPNMIDI_SampleType_F32,
    sizeof(float),
    2 * sizeof(float)
};

#if (LIBVLC_VERSION_MAJOR >= 3)
static int DecodeBlock (decoder_t *p_dec, block_t *p_block);
#else
static block_t *DecodeBlock (decoder_t *p_dec, block_t **pp_block);
#endif
static void Flush (decoder_t *);

static int Open (vlc_object_t *p_this)
{
    decoder_t *p_dec = (decoder_t *)p_this;

    if (p_dec->fmt_in.i_codec != VLC_CODEC_MIDI)
        return VLC_EGENERIC;

    if (!var_InheritBool(p_this, CONFIG_PREFIX "enable"))
        return VLC_EGENERIC;

    decoder_sys_t *p_sys = malloc (sizeof (*p_sys));
    if (unlikely(p_sys == NULL))
        return VLC_ENOMEM;

    p_sys->sample_rate = var_InheritInteger (p_this, CONFIG_PREFIX "sample-rate");
    p_sys->synth = opn2_init( p_sys->sample_rate );

    opn2_switchEmulator(p_sys->synth, var_InheritInteger(p_this, CONFIG_PREFIX "emulator-type"));

    opn2_setNumChips(p_sys->synth, (int)var_InheritInteger(p_this, CONFIG_PREFIX "emulated-chips"));

    opn2_setVolumeRangeModel(p_sys->synth, var_InheritInteger(p_this, CONFIG_PREFIX "volume-model"));

    opn2_setChannelAllocMode(p_sys->synth, var_InheritInteger(p_this, CONFIG_PREFIX "channel-allocation"));

    opn2_setSoftPanEnabled(p_sys->synth, var_InheritBool(p_this, CONFIG_PREFIX "full-panning"));

    opn2_setFullRangeBrightness(p_sys->synth, var_InheritBool(p_this, CONFIG_PREFIX "full-range-brightness"));

    opn2_setAutoArpeggio(p_sys->synth, var_InheritBool(p_this, CONFIG_PREFIX "enable-auto-arpeggio"));

    char *font_path = var_InheritString (p_this, CONFIG_PREFIX "custombank");
    if (font_path != NULL)
    {
        msg_Dbg (p_this, "loading custom bank file %s", font_path);
        if (opn2_openBankFile(p_sys->synth, font_path))
            msg_Err (p_this, "cannot load custom bank file %s: %s", font_path, opn2_errorInfo(p_sys->synth));
        free (font_path);
    }
    else
    {
        if (opn2_openBankData(p_sys->synth, g_xg_wopn_bank, sizeof(g_xg_wopn_bank)))
        {
            msg_Err (p_this, "cannot load default bank file: %s", opn2_errorInfo(p_sys->synth));
#if (LIBVLC_VERSION_MAJOR < 3)
#define vlc_dialog_display_error dialog_Fatal
#endif
            vlc_dialog_display_error (p_this, _("MIDI synthesis not set up"),
                _("A bank file (.WOPN) is required for MIDI synthesis.\n"
                  "Please install a custom bank and configure it "
                  "from the VLC preferences "
                  "(Input / Codecs > Audio codecs > OPNMIDI).\n"));
            opn2_close (p_sys->synth);
            free (p_sys);
            return VLC_EGENERIC;
        }
    }

    p_dec->fmt_out.i_cat = AUDIO_ES;

    p_dec->fmt_out.audio.i_rate = (unsigned)p_sys->sample_rate;
    p_dec->fmt_out.audio.i_channels = 2;
    p_dec->fmt_out.audio.i_physical_channels = AOUT_CHAN_LEFT | AOUT_CHAN_RIGHT;

    p_dec->fmt_out.i_codec = VLC_CODEC_F32L;
    p_dec->fmt_out.audio.i_bitspersample = 32;
    date_Init (&p_sys->end_date, p_dec->fmt_out.audio.i_rate, 1);
    date_Set (&p_sys->end_date, 0);

    p_dec->p_sys = p_sys;
#if (LIBVLC_VERSION_MAJOR >= 3)
    p_dec->pf_decode = DecodeBlock;
    p_dec->pf_flush = Flush;
#else
    p_dec->pf_decode_audio = DecodeBlock;
#endif
    return VLC_SUCCESS;
}


static void Close (vlc_object_t *p_this)
{
    decoder_sys_t *p_sys = ((decoder_t *)p_this)->p_sys;

    opn2_close(p_sys->synth);
    free (p_sys);
}

static void Flush (decoder_t *p_dec)
{
    decoder_sys_t *p_sys = p_dec->p_sys;

#if (LIBVLC_VERSION_MAJOR >= 3)
    date_Set (&p_sys->end_date, VLC_TS_INVALID);
#else
    date_Set (&p_sys->end_date, 0);
#endif
    opn2_panic(p_sys->synth);
}


#if (LIBVLC_VERSION_MAJOR >= 3)
static int DecodeBlock (decoder_t *p_dec, block_t *p_block)
{
    size_t it;
    decoder_sys_t *p_sys = p_dec->p_sys;
    block_t *p_out = NULL;

#else
static block_t *DecodeBlock (decoder_t *p_dec, block_t **pp_block)
{
    size_t it;
    block_t *p_block;
    decoder_sys_t *p_sys = p_dec->p_sys;
    block_t *p_out = NULL;
    if (pp_block == NULL)
        return NULL;
    p_block = *pp_block;
#endif

    if (p_block == NULL)
    {
#if (LIBVLC_VERSION_MAJOR >= 3)
        return VLCDEC_SUCCESS;
#else
        return p_out;
#endif
    }

#if (LIBVLC_VERSION_MAJOR < 3)
    *pp_block = NULL;
#endif

    if (p_block->i_flags & (BLOCK_FLAG_DISCONTINUITY|BLOCK_FLAG_CORRUPTED))
    {
#if (LIBVLC_VERSION_MAJOR >= 3)
        Flush (p_dec);
        if (p_block->i_flags & BLOCK_FLAG_CORRUPTED)
        {
            block_Release(p_block);
            return VLCDEC_SUCCESS;
        }
#else
        Flush (p_dec);
#endif
    }

    if (p_block->i_pts > VLC_TS_INVALID
        && !date_Get (&p_sys->end_date))
        date_Set (&p_sys->end_date, p_block->i_pts);
    else
    if (p_block->i_pts < date_Get (&p_sys->end_date))
    {
        msg_Warn (p_dec, "MIDI message in the past?");
        goto drop;
    }

    if (p_block->i_buffer < 1)
        goto drop;

    uint8_t event = p_block->p_buffer[0];
    uint8_t channel = p_block->p_buffer[0] & 0xf;
    event &= 0xF0;

    if (event == 0xF0)
        switch (channel)
        {
            case 0:
                if (p_block->p_buffer[p_block->i_buffer - 1] != 0xF7)
                {
            case 7:
                    msg_Warn (p_dec, "fragmented SysEx not implemented");
                    goto drop;
                }
                opn2_rt_systemExclusive(p_sys->synth,
                                        (const OPN2_UInt8 *)p_block->p_buffer,
                                        p_block->i_buffer);
                break;
            case 0xF:
                opn2_rt_resetState(p_sys->synth);
                break;
        }

    uint8_t p1 = (p_block->i_buffer > 1) ? (p_block->p_buffer[1] & 0x7f) : 0;
    uint8_t p2 = (p_block->i_buffer > 2) ? (p_block->p_buffer[2] & 0x7f) : 0;

    switch (event & 0xF0)
    {
        case 0x80:
            opn2_rt_noteOff(p_sys->synth, channel, p1);
            break;
        case 0x90:
            opn2_rt_noteOn(p_sys->synth, channel, p1, p2);
            break;
        case 0xA0:
            opn2_rt_noteAfterTouch(p_sys->synth, channel, p1, p2);
            break;
        case 0xB0:
            opn2_rt_controllerChange(p_sys->synth, channel, p1, p2);
            break;
        case 0xC0:
            opn2_rt_patchChange(p_sys->synth, channel, p1);
            break;
        case 0xD0:
            opn2_rt_channelAfterTouch(p_sys->synth, channel, p1);
            break;
        case 0xE0:
            opn2_rt_pitchBendML(p_sys->synth, channel, p2, p1);
            break;
    }

    unsigned samples =
        (p_block->i_pts - date_Get (&p_sys->end_date)) * 441 / 10000;
    if (samples == 0)
        goto drop;

#if (LIBVLC_VERSION_MAJOR >= 3)
    if (decoder_UpdateAudioFormat (p_dec))
        goto drop;
#endif

    p_out = decoder_NewAudioBuffer (p_dec, samples);
    if (p_out == NULL)
        goto drop;

    p_out->i_pts = date_Get(&p_sys->end_date);
    samples = (unsigned)opn2_generateFormat(p_sys->synth, (int)samples * 2,
                                  (OPN2_UInt8*)p_out->p_buffer,
                                  (OPN2_UInt8*)(p_out->p_buffer + g_output_format.containerSize),
                                  &g_output_format);

    for (it = 0; it < samples; ++it)
        ((float*)p_out->p_buffer)[it] *= 2.0f;

    samples /= 2;
    p_out->i_length = date_Increment (&p_sys->end_date, samples) - p_out->i_pts;

drop:
    block_Release (p_block);
#if (LIBVLC_VERSION_MAJOR >= 3)
    if (p_out != NULL)
        decoder_QueueAudio (p_dec, p_out);
    return VLCDEC_SUCCESS;
#else
    return p_out;
#endif
}
