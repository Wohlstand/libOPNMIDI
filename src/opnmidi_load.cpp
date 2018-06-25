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

bool OPNMIDIplay::LoadBank(const std::string &filename)
{
    FileAndMemReader file;
    file.openFile(filename.c_str());
    return LoadBank(file);
}

bool OPNMIDIplay::LoadBank(const void *data, size_t size)
{
    FileAndMemReader file;
    file.openData(data, (size_t)size);
    return LoadBank(file);
}

size_t readU16BE(FileAndMemReader &fr, uint16_t &out)
{
    uint8_t arr[2];
    size_t ret = fr.read(arr, 1, 2);
    out = arr[1];
    out |= ((arr[0] << 8) & 0xFF00);
    return ret;
}

size_t readS16BE(FileAndMemReader &fr, int16_t &out)
{
    uint8_t arr[2];
    size_t ret = fr.read(arr, 1, 2);
    out = *reinterpret_cast<signed char *>(&arr[0]);
    out *= 1 << 8;
    out |= arr[1];
    return ret;
}

int16_t toSint16BE(uint8_t *arr)
{
    int16_t num = *reinterpret_cast<const int8_t *>(&arr[0]);
    num *= 1 << 8;
    num |= arr[1];
    return num;
}

static uint16_t toUint16LE(const uint8_t *arr)
{
    uint16_t num = arr[0];
    num |= ((arr[1] << 8) & 0xFF00);
    return num;
}

static uint16_t toUint16BE(const uint8_t *arr)
{
    uint16_t num = arr[1];
    num |= ((arr[0] << 8) & 0xFF00);
    return num;
}


static const char *wopn2_magic1 = "WOPN2-BANK\0";
static const char *wopn2_magic2 = "WOPN2-B2NK\0";

#define WOPL_INST_SIZE_V1 65
#define WOPL_INST_SIZE_V2 69

static const uint16_t latest_version = 2;

bool OPNMIDIplay::LoadBank(FileAndMemReader &fr)
{
    size_t  fsize;
    ADL_UNUSED(fsize);
    if(!fr.isValid())
    {
        errorStringOut = "Can't load bank file: Invalid data stream!";
        return false;
    }

    char magic[32];
    std::memset(magic, 0, 32);
    uint16_t version = 1;

    uint16_t count_melodic_banks     = 1;
    uint16_t count_percussive_banks   = 1;

    if(fr.read(magic, 1, 11) != 11)
    {
        errorStringOut = "Can't load bank file: Can't read magic number!";
        return false;
    }

    bool is1 = std::strncmp(magic, wopn2_magic1, 11) == 0;
    bool is2 = std::strncmp(magic, wopn2_magic2, 11) == 0;

    if(!is1 && !is2)
    {
        errorStringOut = "Can't load bank file: Invalid magic number!";
        return false;
    }

    if(is2)
    {
        uint8_t ver[2];
        if(fr.read(ver, 1, 2) != 2)
        {
            errorStringOut = "Can't load bank file: Can't read version number!";
            return false;
        }
        version = toUint16LE(ver);
        if(version < 2 || version > latest_version)
        {
            errorStringOut = "Can't load bank file: unsupported WOPN version!";
            return false;
        }
    }

    m_synth.m_insBanks.clear();
    if((readU16BE(fr, count_melodic_banks) != 2) || (readU16BE(fr, count_percussive_banks) != 2))
    {
        errorStringOut = "Can't load bank file: Can't read count of banks!";
        return false;
    }

    if((count_melodic_banks < 1) || (count_percussive_banks < 1))
    {
        errorStringOut = "Custom bank: Too few banks in this file!";
        return false;
    }

    if(fr.read(&m_synth.m_regLFOSetup, 1, 1) != 1)
    {
        errorStringOut = "Can't load bank file: Can't read LFO registry state!";
        return false;
    }

    m_synth.m_insBanks.clear();

    std::vector<OPN2::Bank *> banks;
    banks.reserve(count_melodic_banks + count_percussive_banks);

    if(version >= 2)//Read bank meta-entries
    {
        for(uint16_t i = 0; i < count_melodic_banks; i++)
        {
            uint8_t bank_meta[34];
            if(fr.read(bank_meta, 1, 34) != 34)
            {
                m_synth.m_insBanks.clear();
                errorStringOut = "Custom bank: Fail to read melodic bank meta-data!";
                return false;
            }
            size_t bankno = size_t(bank_meta[33]) * 256 + size_t(bank_meta[32]);
            OPN2::Bank &bank = m_synth.m_insBanks[bankno];
            //strncpy(bank.name, char_p(bank_meta), 32);
            banks.push_back(&bank);
        }

        for(uint16_t i = 0; i < count_percussive_banks; i++)
        {
            uint8_t bank_meta[34];
            if(fr.read(bank_meta, 1, 34) != 34)
            {
                m_synth.m_insBanks.clear();
                errorStringOut = "Custom bank: Fail to read percussion bank meta-data!";
                return false;
            }
            size_t bankno = size_t(bank_meta[33]) * 256 + size_t(bank_meta[32]) + OPN2::PercussionTag;
            OPN2::Bank &bank = m_synth.m_insBanks[bankno];
            //strncpy(bank.name, char_p(bank_meta), 32);
            banks.push_back(&bank);
        }
    }

    size_t total = 128 * m_synth.m_insBanks.size();

    for(size_t i = 0; i < total; i++)
    {
        opnInstMeta2 &meta = banks[i / 128]->ins[i % 128];
        opnInstData &data = meta.opn[0];
        uint8_t idata[WOPL_INST_SIZE_V2];

        size_t readSize = version >= 2 ? WOPL_INST_SIZE_V2 : WOPL_INST_SIZE_V1;
        if(fr.read(idata, 1, readSize) != readSize)
        {
            m_synth.m_insBanks.clear();
            errorStringOut = "Can't load bank file: Failed to read instrument data";
            return false;
        }
        data.finetune = toSint16BE(idata + 32);
        //Percussion instrument note number or a "fixed note sound"
        meta.tone  = idata[34];
        data.fbalg = idata[35];
        data.lfosens = idata[36];
        for(size_t op = 0; op < 4; op++)
        {
            size_t off = 37 + op * 7;
            std::memcpy(data.OPS[op].data, idata + off, 7);
        }

        meta.flags = 0;
        if(version >= 2)
        {
            meta.ms_sound_kon   = toUint16BE(idata + 65);
            meta.ms_sound_koff  = toUint16BE(idata + 67);
            if((meta.ms_sound_kon == 0) && (meta.ms_sound_koff == 0))
                meta.flags |= opnInstMeta::Flag_NoSound;
        }
        else
        {
            meta.ms_sound_kon   = 1000;
            meta.ms_sound_koff  = 500;
        }

        meta.opn[1] = meta.opn[0];

        /* Junk, delete later */
        meta.fine_tune      = 0.0;
        /* Junk, delete later */
    }

    applySetup();

    return true;
}

#ifndef OPNMIDI_DISABLE_MIDI_SEQUENCER

bool OPNMIDIplay::LoadMIDI_pre()
{
    if(m_synth.m_insBanks.empty())
    {
        errorStringOut = "Bank is not set! Please load any instruments bank by using of adl_openBankFile() or adl_openBankData() functions!";
        return false;
    }

    /**** Set all properties BEFORE starting of actial file reading! ****/
    resetMIDI();
    applySetup();

    return true;
}

bool OPNMIDIplay::LoadMIDI_post()
{
    MidiSequencer::FileFormat format = m_sequencer.getFormat();
    if(format == MidiSequencer::Format_CMF)
    {
        errorStringOut = "OPNMIDI doesn't supports CMF, use ADLMIDI to play this file!";
        /* As joke, why not to try implemented the converter of patches from OPL3 into OPN2? */
        return false;
    }
    else if(format == MidiSequencer::Format_RSXX)
    {
        //opl.CartoonersVolumes = true;
        m_synth.m_musicMode     = OPN2::MODE_RSXX;
        m_synth.m_volumeScale   = OPN2::VOLUME_Generic;
    }
    else if(format == MidiSequencer::Format_IMF)
    {
        errorStringOut = "OPNMIDI doesn't supports IMF, use ADLMIDI to play this file!";
        /* Same as for CMF */
        return false;
    }

    m_synth.reset(m_setup.emulator, m_setup.PCM_RATE, this); // Reset OPN2 chip
    m_chipChannels.clear();
    m_chipChannels.resize(m_synth.m_numChannels);

    return true;
}


bool OPNMIDIplay::LoadMIDI(const std::string &filename)
{
    FileAndMemReader file;
    file.openFile(filename.c_str());
    if(!LoadMIDI_pre())
        return false;
    if(!m_sequencer.loadMIDI(file))
    {
        errorStringOut = m_sequencer.getErrorString();
        return false;
    }
    if(!LoadMIDI_post())
        return false;
    return true;
}

bool OPNMIDIplay::LoadMIDI(const void *data, size_t size)
{
    FileAndMemReader file;
    file.openData(data, size);
    if(!LoadMIDI_pre())
        return false;
    if(!m_sequencer.loadMIDI(file))
    {
        errorStringOut = m_sequencer.getErrorString();
        return false;
    }
    if(!LoadMIDI_post())
        return false;
    return true;
}

#endif //OPNMIDI_DISABLE_MIDI_SEQUENCER
