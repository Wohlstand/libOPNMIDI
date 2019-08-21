/*
 * Interfaces over Yamaha OPN2 (YM2612) chip emulators
 *
 * Copyright (c) 2017-2019 Vitaly Novichkov (Wohlstand)
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

#ifndef VGM_FILE_DUMPER_H
#define VGM_FILE_DUMPER_H

#include "opn_chip_base.h"

class Ym2612_Emu;
class VGMFileDumper final : public OPNChipBaseBufferedT<VGMFileDumper>
{
    FILE *m_output;
    uint32_t m_bytes_written;
    uint32_t m_samples_written;
    uint32_t m_actual_rate;
    uint64_t m_delay;
    int m_chip_index;

    struct VgmHead
    {
        char magic[4];
        uint32_t eof_offset;
        uint32_t version;
        uint32_t clock_sn76489;
        uint32_t clock_ym2413;
        uint32_t offset_gd3;
        uint32_t total_samples;
        uint32_t offset_loop;
        uint32_t loop_samples;
        uint32_t rate;
        uint16_t feedback_sn76489;
        uint8_t  shift_register_width_sn76489;
        uint8_t  flags_sn76489;
        uint32_t clock_ym2612;
        uint32_t clock_ym2151;
        uint32_t offset_data;
    };
    VgmHead m_vgm_head;
public:
    explicit VGMFileDumper(OPNFamily f);
    ~VGMFileDumper() override;

    bool canRunAtPcmRate() const override { return true; }
    void setRate(uint32_t rate, uint32_t clock) override;
    void reset() override;
    void writeReg(uint32_t port, uint16_t addr, uint8_t data) override;
    void writePan(uint16_t chan, uint8_t data) override;
    void nativePreGenerate() override {}
    void nativePostGenerate() override {}
    void nativeGenerateN(int16_t *output, size_t frames) override;
    const char *emulatorName() override;
};

#endif // VGM_FILE_DUMPER_H
