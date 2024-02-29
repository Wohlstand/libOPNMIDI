/*
 * Interfaces over Yamaha OPN2 (YM2612) chip emulators
 *
 * Copyright (c) 2017-2024 Vitaly Novichkov (Wohlstand)
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#include "opn_wave_synth.h"
#include <string.h>
#include <stdio.h>
#include <cmath>


void OPNWaveSynth::releaseChannel(int ch)
{
    if(m_channelsUsed == 0)
        return;

    m_channels[ch].cur_chunk = -1;
    m_channels[ch].len = 0;
    m_channels[ch].pos = 0;

    if(m_channelsUsed > 1)
        memcpy(&m_channels[ch], &m_channels[m_channelsUsed - 1], sizeof(WaveChannel));

    m_channelsUsed--;
}

OPNWaveSynth::OPNWaveSynth()
{
    resetAllMaps();
    resetChans();

    m_rate = 11025;
    m_channelsUsed = 0;

    loadChunkFromFile("/home/vitaly/_git_repos/libOPNMIDI/test/pcm-samples/kick.raw", 0, 35, true);
    loadChunkFromFile("/home/vitaly/_git_repos/libOPNMIDI/test/pcm-samples/kick.raw", 0, 36, true);
    loadChunkFromFile("/home/vitaly/_git_repos/libOPNMIDI/test/pcm-samples/snare.raw", 0, 38, true);
    loadChunkFromFile("/home/vitaly/_git_repos/libOPNMIDI/test/pcm-samples/snare2.raw", 0, 40, true);

    loadChunkFromFile("/home/vitaly/_git_repos/libOPNMIDI/test/pcm-samples/drums1kick.raw", 16, 35, true);
    loadChunkFromFile("/home/vitaly/_git_repos/libOPNMIDI/test/pcm-samples/drums3kick.raw", 16, 36, true);
    loadChunkFromFile("/home/vitaly/_git_repos/libOPNMIDI/test/pcm-samples/drums3snare.raw", 16, 38, true);
    loadChunkFromFile("/home/vitaly/_git_repos/libOPNMIDI/test/pcm-samples/drums1snare.raw", 16, 40, true);

    loadChunkFromFile("/home/vitaly/_git_repos/libOPNMIDI/test/pcm-samples/orchestraKik.raw", 48, 35, true);
    loadChunkFromFile("/home/vitaly/_git_repos/libOPNMIDI/test/pcm-samples/orchestraBassDrum.raw", 48, 36, true);
    loadChunkFromFile("/home/vitaly/_git_repos/libOPNMIDI/test/pcm-samples/orchestraSnare.raw", 48, 38, true);
    loadChunkFromFile("/home/vitaly/_git_repos/libOPNMIDI/test/pcm-samples/orchestraSnare.raw", 48, 40, true);
}

OPNWaveSynth::~OPNWaveSynth()
{}

void OPNWaveSynth::resetChans()
{
    memset(m_channels, 0, sizeof(m_channels));
    for(size_t i = 0; i < m_channelsNum; ++i)
        m_channels[i].cur_chunk = -1;
}

void OPNWaveSynth::resetAllMaps()
{
    m_channelsMax = 1;

    memset(m_percMap, 0, sizeof(m_percMap));
    memset(m_melodicUse, 0, sizeof(m_melodicUse));

    memset(m_curBank, 0, sizeof(m_curBank));
    memset(m_curPatch, 0, sizeof(m_curPatch));
    memset(m_curIsDrum, 0, sizeof(m_curIsDrum));

    for(size_t i = 0; i < 16; ++i)
    {
        m_volumes[i] = 1.0f;
        m_expressions[i] = 1.0f;
    }

    for(size_t chan = 0; chan < 16; ++chan)
    {
        for(size_t i = 0; i < 128; ++i)
            m_curDrumChunks[chan][i] = -1;
    }
}

void OPNWaveSynth::setMaxChannels(size_t chans)
{
    m_channelsMax = chans;
    if(m_channelsMax > m_channelsNum)
        m_channelsMax = m_channelsNum;
}

void OPNWaveSynth::loadChunkFromFile(const std::string &file, int bank, int ins, bool isPerc)
{
    FILE *f = fopen(file.c_str(), "rb");
    if(!f)
        return;

    std::vector<uint8_t > dump;
    fseek(f, 0, SEEK_END);
    size_t size = (size_t)ftell(f);
    fseek(f, 0, SEEK_SET);

    dump.resize(size);
    fread(&dump[0], 1, size, f);
    fclose(f);

    loadChunkFromData(&dump[0], size, bank, ins, isPerc);
}

void OPNWaveSynth::loadChunkFromData(uint8_t *data, size_t size, int bank, int ins, bool isPerc)
{
    WaveChunk chunk;
    chunk.pcm.resize(size);
    memcpy(&chunk.pcm[0], data, size);
    chunk.bank = bank;
    chunk.ins = ins;
    chunk.is_drum = isPerc;
    chunks.push_back(chunk);
}

void OPNWaveSynth::changePatch(int channel, int patch, bool isDrum)
{
    channel %= 16;
    m_curPatch[channel] = patch;
    m_curIsDrum[channel] = isDrum;

    for(size_t i = 0; i < 128; ++i)
        m_curDrumChunks[channel][i] = -1;

    memset(m_percMap[channel], 0, sizeof(m_percMap[channel]));

    if(isDrum)
    {
        for(size_t i = 0; i < chunks.size(); ++i)
        {
            const WaveChunk &c = chunks[i];
            if(m_curPatch[channel] == c.bank)
            {
                m_curDrumChunks[channel][c.ins] = i;
                m_percMap[channel][c.ins] = true;
            }
        }
    }
}

void OPNWaveSynth::changeBank(int channel, int bank)
{
    channel %= 16;
    m_curBank[channel] = bank;
}

bool OPNWaveSynth::hasWave(uint8_t channel, uint8_t note)
{
    channel %= 16;

    bool isPercussion = (channel == 9) || m_curIsDrum[channel];

    if(isPercussion)
        return m_percMap[channel][note];
    else
        return m_melodicUse[channel];
}

void OPNWaveSynth::noteOn(int channel, int note, uint8_t velocity)
{
    channel %= 16;

    bool isDrum = m_curIsDrum[channel];
    int reUse = -1;

    if(velocity > 127)
        velocity = 127;

    if(isDrum)
    {
        int drum = m_curDrumChunks[channel][note];

        if(drum < 0 || drum >= (int)chunks.size())
            return; // No drum here

        // Find the channel with the same chunk and re-use it
        for(int i = 0; i < m_channelsUsed; ++i)
        {
            if(m_channels[i].cur_chunk == drum)
            {
                reUse = i;
                break;
            }
        }

        if(reUse < 0 && m_channelsUsed == m_channelsNum)
            return; // No free channels

        WaveChannel &c = m_channels[reUse < 0 ? m_channelsUsed++ : reUse];
        c.cur_chunk = drum;
        c.pos = 0;
        c.len = chunks[c.cur_chunk].pcm.size();
        c.midi_chan = channel;
        c.vel = (int)std::floor(127.0f * std::sqrt(((float)velocity) * (1.f / 127.f))) / 127.f;

        // if(m_channelsUsed < 1)
        //     m_channelsUsed++;
    }
}

void OPNWaveSynth::fetchPcm(int32_t *buffer, int samples, int flag_mixing, size_t outNum)
{
    if(m_channelsUsed == 0)
        return; // Nothing to play

    if(!flag_mixing)
        memset(buffer, 0, sizeof(int32_t) * samples * 2);

    for(int i = (outNum % m_channelsMax); i < (int)m_channelsUsed; i += m_channelsMax)
    {
        WaveChannel &c = m_channels[i];
        int len_left = c.len - c.pos;
        int32_t *dst = buffer;
        uint8_t *src = &chunks[c.cur_chunk].pcm[c.pos];
        int len_req = samples > len_left ? len_left : samples;
        float vol = m_volumes[c.midi_chan] * m_expressions[c.midi_chan];

        c.pos += len_req;

        for(int t = 0; t < samples; t++)
        {
            int32_t mid = (int32_t)(*dst) - 128;
            *dst++ = (((mid + ((int32_t)(*src++) - 128)) * c.vel * vol) + 128);
        }

        if(c.pos >= c.len)
            releaseChannel(i--);
    }
}

void OPNWaveSynth::fetchPcmS(void *self, int32_t *buffer, int samples, size_t outNum)
{
    OPNWaveSynth *s = reinterpret_cast<OPNWaveSynth*>(self);
    s->fetchPcm(buffer, samples, 1, outNum);
}
