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

#ifndef OPNWAVESYNTH_H
#define OPNWAVESYNTH_H

#include <stdint.h>
#include <stddef.h>
#include <vector>
#include <string>


class OPNWaveSynth
{
    bool m_percMap[16][128];
    bool m_melodicUse[16];

    int m_curBank[16];
    int m_curPatch[16];
    bool m_curIsDrum[16];
    int m_curDrumChunks[16][128];

    float m_volumes[16];
    float m_expressions[16];

    struct WaveChunk
    {
        std::vector<uint8_t > pcm;
        uint8_t bank;
        uint8_t ins;
        bool is_drum;
    };

    std::vector<WaveChunk > chunks;

    uint16_t m_rate;

    struct WaveChannel
    {
        int cur_chunk;
        size_t pos;
        size_t len;
        size_t midi_chan;
        float vel;
    };

    static const size_t m_channelsNum = 16;
    WaveChannel m_channels[m_channelsNum];
    int m_channelsUsed;

    void releaseChannel(int ch);

public:
    OPNWaveSynth();
    virtual ~OPNWaveSynth();

    void resetChans();
    void resetAllMaps();

    void loadChunkFromFile(const std::string &file, int bank, int ins, bool isPerc);
    void loadChunkFromData(uint8_t *data, size_t size, int bank, int ins, bool isPerc);

    void changePatch(int channel, int patch, bool isDrum);
    void changeBank(int channel, int bank);

    bool hasWave(uint8_t channel, uint8_t note);

    void noteOn(int channel, int note, uint8_t velocity);

    void fetchPcm(int32_t *buffer, int samples, int flag_mixing);

    static void fetchPcmS(void *self, int32_t *buffer, int samples);
};

#endif // OPNWAVESYNTH_H
