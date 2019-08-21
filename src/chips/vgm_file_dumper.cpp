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

#include "vgm_file_dumper.h"
#include <cstring>
#include <cassert>

//! FIXME: Replace this ugly crap with proper public call
static const char *g_vgm_path = "kek.vgm";
extern "C" void opn2_set_vgm_out_path(const char *path)
{
    g_vgm_path = path;
}


VGMFileDumper::VGMFileDumper(OPNFamily f)
    : OPNChipBaseBufferedT(f)
{
    m_bytes_written = 0;
    m_samples_written = 0;
    setRate(m_rate, m_clock);
    m_output = std::fopen(g_vgm_path, "wb");
    assert(m_output);
    std::memset(&m_vgm_head, 0, sizeof(VgmHead));
    std::memcpy(m_vgm_head.magic, "Vgm ", 4);
    m_vgm_head.version = 0x00000110;
    std::fseek(m_output, 0x40, SEEK_SET);
}

VGMFileDumper::~VGMFileDumper()
{
    uint8_t out[1];
    out[0] = 0x66;// end of sound data
    std::fwrite(&out, 1, 1, m_output);
    m_bytes_written += 1;

    std::fseek(m_output, 0x00, SEEK_SET);
    m_vgm_head.total_samples = m_bytes_written;
    m_vgm_head.offset_loop = 0x0040;
    m_vgm_head.loop_samples = m_samples_written;
    m_vgm_head.eof_offset = (0x40 + m_bytes_written - 4);
    //! FIXME: Make proper endianess suporrt
    std::fwrite(&m_vgm_head, 1, sizeof(VgmHead), m_output);
    fclose(m_output);
}

void VGMFileDumper::setRate(uint32_t rate, uint32_t clock)
{
    OPNChipBaseBufferedT::setRate(rate, clock);
    m_actual_rate = isRunningAtPcmRate() ? rate : nativeRate();
    m_vgm_head.clock_ym2612 = m_clock;
}

void VGMFileDumper::reset()
{
    OPNChipBaseBufferedT::reset();
    std::fseek(m_output, 0x40, SEEK_SET);
    m_samples_written = 0;
}

void VGMFileDumper::writeReg(uint32_t port, uint16_t addr, uint8_t data)
{
    uint8_t out[3];
    switch (port)
    {
    case 0:
        out[0] = 0x52;
        break;
    case 1:
        out[0] = 0x53;
        break;
    }
    out[1] = (uint8_t)addr;
    out[2] = (uint8_t)data;

    std::fwrite(&out, 1, 3, m_output);
    m_bytes_written += 3;
}

void VGMFileDumper::writePan(uint16_t /*chan*/, uint8_t /*data*/)
{}

void VGMFileDumper::nativeGenerateN(int16_t *output, size_t frames)
{
    std::memset(output, 0, frames * sizeof(int16_t) * 2);
    uint8_t out[3];
    size_t wait_sec = size_t(frames * (44100.0 / double(m_actual_rate)));
    out[0] = 0x61;
    out[1] = wait_sec & 0xFF;
    out[2] = (wait_sec >> 8) & 0xFF;
    std::fwrite(&out, 1, 3, m_output);
    m_samples_written++;
    m_bytes_written += 3;
}

const char *VGMFileDumper::emulatorName()
{
    return "VGM Writer";
}
