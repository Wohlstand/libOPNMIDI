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

#include "opnmidi_private.hpp"

#if defined(OPNMIDI_DISABLE_NUKED_EMULATOR) && defined(OPNMIDI_DISABLE_MAME_EMULATOR) && \
    defined(OPNMIDI_DISABLE_GENS_EMULATOR) && defined(OPNMIDI_DISABLE_GX_EMULATOR)
#error "No emulators enabled. You must enable at least one emulator to use this library!"
#endif

// Nuked OPN2 emulator, Most accurate, but requires the powerful CPU
#ifndef OPNMIDI_DISABLE_NUKED_EMULATOR
#include "chips/nuked_opn2.h"
#endif

// MAME YM2612 emulator, Well-accurate and fast
#ifndef OPNMIDI_DISABLE_MAME_EMULATOR
#include "chips/mame_opn2.h"
#endif

// GENS 2.10 emulator, very outdated and inaccurate, but gives the best performance
#ifndef OPNMIDI_DISABLE_GENS_EMULATOR
#include "chips/gens_opn2.h"
#endif

// Genesis Plus GX emulator, Variant of MAME with enhancements
#ifndef OPNMIDI_DISABLE_GX_EMULATOR
#include "chips/gx_opn2.h"
#endif


static const uint32_t g_noteChannelsMap[6] = { 0, 1, 2, 4, 5, 6 };

static inline void getOpnChannel(size_t     in_channel,
                                 size_t     &out_chip,
                                 uint8_t    &out_port,
                                 uint32_t   &out_ch)
{
    out_chip    = in_channel / 6;
    size_t ch4 = in_channel % 6;
    out_port = ((ch4 < 3) ? 0 : 1);
    out_ch = static_cast<uint32_t>(ch4 % 3);
}

static opnInstMeta2 makeEmptyInstrument()
{
    opnInstMeta2 ins;
    memset(&ins, 0, sizeof(opnInstMeta2));
    ins.flags = opnInstMeta::Flag_NoSound;
    return ins;
}

const opnInstMeta2 OPN2::m_emptyInstrument = makeEmptyInstrument();

OPN2::OPN2() :
    m_regLFOSetup(0),
    m_numChips(1),
    m_musicMode(MODE_MIDI),
    m_volumeScale(VOLUME_Generic)
{
    // Initialize blank instruments banks
    m_insBanks.clear();
}

OPN2::~OPN2()
{
    clearChips();
}

void OPN2::writeReg(size_t chip, uint8_t port, uint8_t index, uint8_t value)
{
    m_chips[chip]->writeReg(port, index, value);
}

void OPN2::writeRegI(size_t chip, uint8_t port, uint32_t index, uint32_t value)
{
    m_chips[chip]->writeReg(port, static_cast<uint8_t>(index), static_cast<uint8_t>(value));
}

void OPN2::noteOff(size_t c)
{
    size_t      chip;
    uint8_t     port;
    uint32_t    cc;
    size_t      ch4 = c % 6;
    getOpnChannel(c, chip, port, cc);
    writeRegI(chip, 0, 0x28, g_noteChannelsMap[ch4]);
}

void OPN2::noteOn(size_t c, double hertz) // Hertz range: 0..131071
{
    size_t      chip;
    uint8_t     port;
    uint32_t    cc;
    size_t      ch4 = c % 6;
    getOpnChannel(c, chip, port, cc);

    uint32_t x2 = 0x0000;

    if(hertz < 0 || hertz > 262143) // Avoid infinite loop
        return;

    while((hertz >= 1023.75) && (x2 < 0x3800))
    {
        hertz /= 2.0;    // Calculate octave
        x2 += 0x800;
    }

    x2 += static_cast<uint32_t>(hertz + 0.5);
    writeRegI(chip, port, 0xA4 + cc, (x2>>8) & 0xFF);//Set frequency and octave
    writeRegI(chip, port, 0xA0 + cc, x2 & 0xFF);
    writeRegI(chip, 0, 0x28, 0xF0 + g_noteChannelsMap[ch4]);
}

void OPN2::touchNote(size_t c, uint8_t volume, uint8_t brightness)
{
    if(volume > 127) volume = 127;

    size_t      chip;
    uint8_t     port;
    uint32_t    cc;
    getOpnChannel(c, chip, port, cc);

    const opnInstData &adli = m_insCache[c];

    uint8_t op_vol[4] =
    {
        adli.OPS[OPERATOR1].data[1],
        adli.OPS[OPERATOR2].data[1],
        adli.OPS[OPERATOR3].data[1],
        adli.OPS[OPERATOR4].data[1],
    };

    bool alg_do[8][4] =
    {
        /*
         * Yeah, Operator 2 and 3 are seems swapped
         * which we can see in the algorithm 4
         */
        //OP1   OP3   OP2    OP4
        //30    34    38     3C
        {false,false,false,true},//Algorithm #0:  W = 1 * 2 * 3 * 4
        {false,false,false,true},//Algorithm #1:  W = (1 + 2) * 3 * 4
        {false,false,false,true},//Algorithm #2:  W = (1 + (2 * 3)) * 4
        {false,false,false,true},//Algorithm #3:  W = ((1 * 2) + 3) * 4
        {false,false,true, true},//Algorithm #4:  W = (1 * 2) + (3 * 4)
        {false,true ,true ,true},//Algorithm #5:  W = (1 * (2 + 3 + 4)
        {false,true ,true ,true},//Algorithm #6:  W = (1 * 2) + 3 + 4
        {true ,true ,true ,true},//Algorithm #7:  W = 1 + 2 + 3 + 4
    };

    uint8_t alg = adli.fbalg & 0x07;
    for(uint8_t op = 0; op < 4; op++)
    {
        bool do_op = alg_do[alg][op] || m_scaleModulators;
        uint32_t x = op_vol[op];
        uint32_t vol_res = do_op ? (127 - (static_cast<uint32_t>(volume) * (127 - (x & 127)))/127) : x;
        if(brightness != 127)
        {
            brightness = static_cast<uint32_t>(::round(127.0 * ::sqrt((static_cast<double>(brightness)) * (1.0 / 127.0))));
            if(!do_op)
                vol_res = (127 - (brightness * (127 - (static_cast<uint32_t>(vol_res) & 127))) / 127);
        }
        writeRegI(chip, port, 0x40 + cc + (4 * op), vol_res);
    }
    // Correct formula (ST3, AdPlug):
    //   63-((63-(instrvol))/63)*chanvol
    // Reduces to (tested identical):
    //   63 - chanvol + chanvol*instrvol/63
    // Also (slower, floats):
    //   63 + chanvol * (instrvol / 63.0 - 1)
}

void OPN2::setPatch(size_t c, const opnInstData &instrument)
{
    size_t      chip;
    uint8_t     port;
    uint32_t    cc;
    getOpnChannel(c, chip, port, cc);
    m_insCache[c] = instrument;
    for(uint8_t d = 0; d < 7; d++)
    {
        for(uint8_t op = 0; op < 4; op++)
            writeRegI(chip, port, 0x30 + (0x10 * d) + (op * 4) + cc, instrument.OPS[op].data[d]);
    }

    writeRegI(chip, port, 0xB0 + cc, instrument.fbalg);//Feedback/Algorithm
    m_regLFOSens[c] = (m_regLFOSens[c] & 0xC0) | (instrument.lfosens & 0x3F);
    writeRegI(chip, port, 0xB4 + cc, m_regLFOSens[c]);//Panorame and LFO bits
}

void OPN2::setPan(size_t c, uint8_t value)
{
    size_t      chip;
    uint8_t     port;
    uint32_t    cc;
    getOpnChannel(c, chip, port, cc);
    const opnInstData &adli = m_insCache[c];
    uint8_t val = (value & 0xC0) | (adli.lfosens & 0x3F);
    m_regLFOSens[c] = val;
    writeRegI(chip, port, 0xB4 + cc, val);
}

void OPN2::silenceAll() // Silence all OPL channels.
{
    for(size_t c = 0; c < m_numChannels; ++c)
    {
        noteOff(c);
        touchNote(c, 0);
    }
}

void OPN2::setVolumeScaleModel(OPNMIDI_VolumeModels volumeModel)
{
    switch(volumeModel)
    {
    case OPNMIDI_VolumeModel_AUTO://Do nothing until restart playing
        break;

    case OPNMIDI_VolumeModel_Generic:
        m_volumeScale = OPN2::VOLUME_Generic;
        break;

    case OPNMIDI_VolumeModel_NativeOPN2:
        m_volumeScale = OPN2::VOLUME_NATIVE;
        break;

    case OPNMIDI_VolumeModel_DMX:
        m_volumeScale = OPN2::VOLUME_DMX;
        break;

    case OPNMIDI_VolumeModel_APOGEE:
        m_volumeScale = OPN2::VOLUME_APOGEE;
        break;

    case OPNMIDI_VolumeModel_9X:
        m_volumeScale = OPN2::VOLUME_9X;
        break;
    }
}

void OPN2::clearChips()
{
    for(size_t i = 0; i < m_chips.size(); i++)
        m_chips[i].reset(NULL);
    m_chips.clear();
}

void OPN2::reset(int emulator, unsigned long PCM_RATE, void *audioTickHandler)
{
#if !defined(ADLMIDI_AUDIO_TICK_HANDLER)
    ADL_UNUSED(audioTickHandler);
#endif
    clearChips();
    m_insCache.clear();
    m_regLFOSens.clear();
    m_chips.resize(m_numChips, AdlMIDI_SPtr<OPNChipBase>());

    for(size_t i = 0; i < m_chips.size(); i++)
    {
        OPNChipBase *chip;

        switch(emulator)
        {
        default:
#ifndef OPNMIDI_DISABLE_MAME_EMULATOR
        case OPNMIDI_EMU_MAME:
            chip = new MameOPN2;
            break;
#endif
#ifndef OPNMIDI_DISABLE_NUKED_EMULATOR
        case OPNMIDI_EMU_NUKED:
            chip = new NukedOPN2;
            break;
#endif
#ifndef OPNMIDI_DISABLE_GENS_EMULATOR
        case OPNMIDI_EMU_GENS:
            chip = new GensOPN2;
            break;
#endif
#ifndef OPNMIDI_DISABLE_GX_EMULATOR
        case OPNMIDI_EMU_GX:
            chip = new GXOPN2;
            break;
#endif
        }
        m_chips[i].reset(chip);
        chip->setChipId((uint32_t)i);
        chip->setRate((uint32_t)PCM_RATE, 7670454);
        if(m_runAtPcmRate)
            chip->setRunningAtPcmRate(true);
#if defined(ADLMIDI_AUDIO_TICK_HANDLER)
        chip->setAudioTickHandlerInstance(audioTickHandler);
#endif
    }

    m_numChannels = m_numChips * 6;
    m_insCache.resize(m_numChannels,   m_emptyInstrument.opn[0]);
    m_regLFOSens.resize(m_numChannels,    0);

    for(size_t card = 0; card < m_numChips; ++card)
    {
        writeReg(card, 0, 0x22, m_regLFOSetup);//push current LFO state
        writeReg(card, 0, 0x27, 0x00);  //set Channel 3 normal mode
        writeReg(card, 0, 0x2B, 0x00);  //Disable DAC
        //Shut up all channels
        writeReg(card, 0, 0x28, 0x00 ); //Note Off 0 channel
        writeReg(card, 0, 0x28, 0x01 ); //Note Off 1 channel
        writeReg(card, 0, 0x28, 0x02 ); //Note Off 2 channel
        writeReg(card, 0, 0x28, 0x04 ); //Note Off 3 channel
        writeReg(card, 0, 0x28, 0x05 ); //Note Off 4 channel
        writeReg(card, 0, 0x28, 0x06 ); //Note Off 5 channel
    }

    silenceAll();
}
