/*
 * libOPNMIDI is a free Software MIDI synthesizer library with OPN2 (YM2612) emulation
 *
 * MIDI parser and player (Original code from ADLMIDI): Copyright (c) 2010-2014 Joel Yliluoma <bisqwit@iki.fi>
 * OPNMIDI Library and YM2612 support:   Copyright (c) 2017-2023 Vitaly Novichkov <admin@wohlnet.ru>
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

#include "opnmidi_midiplay.hpp"
#include "opnmidi_opn2.hpp"
#include "chips/opn_chip_base.h"

#define TSF_IMPLEMENTATION
#include "chips/tsf/tsf.h"


bool OPNMIDIplay::LoadWaveBank(const std::string &filename)
{
    m_synthTSF.reset(tsf_load_filename(filename.c_str()));
    if(!m_synthTSF.get())
    {
        m_tsfEnabled = false;
        std::memset(m_tsfPercMap, 0, sizeof(m_tsfPercMap));
        std::memset(m_tsfMelodicUse, 0, sizeof(m_tsfMelodicUse));
        errorStringOut = "Failed to load the wavetable bank!";
        return false;
    }

    for(int ch = 0; ch < 16; ch++)
        tsf_channel_set_bank(m_synthTSF.get(), ch, 0);

    tsf_channel_set_bank_preset(m_synthTSF.get(), 9, 128, 0);
    tsf_set_output(m_synthTSF.get(), TSF_MONO /*TSF_STEREO_INTERLEAVED*/, static_cast<int>(11025), -2.0f);

    m_tsfEnabled = true;

    waveReset();

    return true;
}

bool OPNMIDIplay::LoadWaveBank(const void *data, size_t size)
{
    m_synthTSF.reset(tsf_load_memory(data, size));
    if(!m_synthTSF.get())
    {
        m_tsfEnabled = false;
        std::memset(m_tsfPercMap, 0, sizeof(m_tsfPercMap));
        std::memset(m_tsfMelodicUse, 0, sizeof(m_tsfMelodicUse));
        errorStringOut = "Failed to load the wavetable bank!";
        return false;
    }

    for(int ch = 0; ch < 16; ch++)
        tsf_channel_set_bank(m_synthTSF.get(), ch, 0);

    tsf_channel_set_bank_preset(m_synthTSF.get(), 9, 128, 0);
    tsf_set_output(m_synthTSF.get(), TSF_MONO/*TSF_STEREO_INTERLEAVED*/, static_cast<int>(11025), -2.0f);

    m_tsfEnabled = true;

    waveReset();

    return true;
}


void OPNMIDIplay::waveMelMap(int channel)
{
    if(!m_tsfEnabled)
        return;

    channel %= 16;

    MIDIchannel &midiChan = m_midiChannels[channel];
    m_tsfMelodicUse[channel] = false;

    for(int i = 0; i < m_synthTSF.get()->presetNum; ++i)
    {
        struct tsf_preset &p = m_synthTSF.get()->presets[0];
        if(p.bank == midiChan.bank_lsb && p.preset == midiChan.patch)
        {
            m_tsfMelodicUse[channel] = true;
            break;
        }
    }
}

static void s_mapRegion(struct tsf_preset &p, bool *perc)
{
    for(int j = 0; j < p.regionNum; ++j)
    {
        struct tsf_region &r = p.regions[j];
        for(int k = r.lokey; k <= r.hikey; ++k)
            perc[k] = true;
    }
}

void OPNMIDIplay::wavePercMap(int channel, int instrument)
{
    if(!m_tsfEnabled)
        return;

    channel %= 16;
    m_tsfMelodicUse[channel] = false;

    std::memset(m_tsfPercMap[channel], 0, sizeof(m_tsfPercMap[channel]));
    bool *perc = m_tsfPercMap[channel];

    for(int i = 0; i < m_synthTSF.get()->presetNum; ++i)
    {
        struct tsf_preset &p = m_synthTSF.get()->presets[i];

        if(p.bank != 128 || p.preset != instrument)
            continue;

        s_mapRegion(p, perc);
        return;
    }
}

void OPNMIDIplay::waveMap(int channel)
{
    if(!m_tsfEnabled)
        return;

    channel %= 16;
    MIDIchannel &midiChan = m_midiChannels[channel];
    bool isPercussion = channel == 9 || midiChan.is_xg_percussion;

    if(isPercussion)
        wavePercMap(channel, midiChan.patch);
    else
        waveMelMap(channel);
}

bool OPNMIDIplay::useWave(uint8_t channel, uint8_t note)
{
    channel %= 16;

    MIDIchannel &midiChan = m_midiChannels[channel];
    bool isPercussion = (channel == 9) || midiChan.is_xg_percussion;

    if(isPercussion)
        return m_tsfPercMap[channel][note];
    else
        return m_tsfMelodicUse[channel];
}

void OPNMIDIplay::waveReset()
{
    if(!m_tsfEnabled)
        return;

    tsf_reset(m_synthTSF.get());

    for(int c = 0; c < 16; ++c)
    {
        MIDIchannel &midiChan = m_midiChannels[c];
        tsf_channel_set_presetnumber(m_synthTSF.get(), c, midiChan.patch, (c == 9) || midiChan.is_xg_percussion);
        tsf_channel_midi_control(m_synthTSF.get(), c, 1, midiChan.vibrato);
        tsf_channel_midi_control(m_synthTSF.get(), c, 7, midiChan.volume);
        tsf_channel_midi_control(m_synthTSF.get(), c, 10, midiChan.panning);
        tsf_channel_midi_control(m_synthTSF.get(), c, 11, midiChan.expression);
        tsf_channel_midi_control(m_synthTSF.get(), c, 64, midiChan.sustain ? 127 : 0);
        tsf_channel_midi_control(m_synthTSF.get(), c, 67, midiChan.softPedal ? 127 : 0);
        tsf_channel_set_pitchwheel(m_synthTSF.get(), c, midiChan.bend + 8192);
        waveMap(c);
    }

    waveAttach();
}

void OPNMIDIplay::waveAttach()
{
    if(!m_tsfEnabled)
        return;

    if(!m_synth->m_chips.empty() && m_synth->m_chips[0]->enableDAC(true))
        m_synth->m_chips[0]->setFetchPcmCB(&OPNMIDIplay::waveRenderS, this, 11025);
}

void OPNMIDIplay::waveRender(int32_t *buffer, int samples, int flag_mixing)
{
    tsf* f = m_synthTSF.get();
    float outputSamples[TSF_RENDER_SHORTBUFFERBLOCK];
    int channels = (f->outputmode == TSF_MONO ? 1 : 2), maxChannelSamples = TSF_RENDER_SHORTBUFFERBLOCK / channels;
    const float gain = 1.0f;

    while(samples > 0)
    {
        int channelSamples = (samples > maxChannelSamples ? maxChannelSamples : samples);
        int* bufferEnd = buffer + channelSamples * channels;
        float *floatSamples = outputSamples;
        tsf_render_float(f, floatSamples, channelSamples, TSF_FALSE);
        samples -= channelSamples;

        if(flag_mixing)
        {
            while (buffer != bufferEnd)
            {
                float v = *floatSamples++;
                int8_t v8 = (int)(v * 127.5f * gain);
                int vi = *buffer + ((int)v8 + 128 /* * 258*/);
                *buffer++ = vi;
            }
        }
        else
        {
            while (buffer != bufferEnd)
            {
                float v = *floatSamples++;
                int8_t v8 = (int)(v * 127.5f * gain);
                *buffer++ = (int)v8 + 128 /* * 258*/;
            }
        }
    }
}

void OPNMIDIplay::waveRenderS(void *self, int32_t *buffer, int samples)
{
    OPNMIDIplay *s = reinterpret_cast<OPNMIDIplay*>(self);
    s->waveRender(buffer, samples, 1);
}

bool OPNMIDIplay::waveIsPlaying()
{
    bool ret = false;
    tsf* f = m_synthTSF.get();
    struct tsf_voice *v = f->voices, *vEnd = v + f->voiceNum;

    for(; v != vEnd; v++)
        ret |= (v->playingPreset != -1);

    return ret;
}
