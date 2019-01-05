/*
 * Interfaces over Yamaha OPN2 (YM2612) chip emulators
 *
 * Copyright (c) 2018-2019 Vitaly Novichkov (Wohlstand)
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

#include "mame_opna.h"
#include "mamefm/fm.h"
#include "mamefm/2608intf.h"

struct MameOPNA::Impl {
    ym2608_device dev;
    void *chip;
    ssg_callbacks cbssg;
    // callbacks
    static uint8_t cbInternalReadByte(device_t *, offs_t) { return 0; }
    static uint8_t cbExternalReadByte(device_t *, offs_t) { return 0; }
    static void cbExternalWriteByte(device_t *, offs_t, uint8_t) {}
    static void cbHandleTimer(device_t *, int, int, int) {}
    static void cbHandleIRQ(device_t *, int) {}
    static void cbSsgSetClock(device_t *, int) {}
    static void cbSsgWrite(device_t *, int, int) {}
    static int cbSsgRead(device_t *) { return 0; }
    static void cbSsgReset(device_t *) {}
};

MameOPNA::MameOPNA(OPNFamily f)
    : OPNChipBaseBufferedT(f), impl(new Impl)
{
    impl->chip = NULL;
    impl->cbssg.set_clock = &Impl::cbSsgSetClock;
    impl->cbssg.write = &Impl::cbSsgWrite;
    impl->cbssg.read = &Impl::cbSsgRead;
    impl->cbssg.reset = &Impl::cbSsgReset;
    setRate(m_rate, m_clock);
}

MameOPNA::~MameOPNA()
{
    ym2608_shutdown(impl->chip);
    delete impl;
}

void MameOPNA::setRate(uint32_t rate, uint32_t clock)
{
    OPNChipBaseBufferedT::setRate(rate, clock);
    if(impl->chip)
        ym2608_shutdown(impl->chip);

    uint32_t chipRate = isRunningAtPcmRate() ? rate : nativeRate();
    ym2608_device *device = &impl->dev;
    void *chip = impl->chip = ym2608_init(
        device, (int)clock, (int)chipRate,
        &Impl::cbInternalReadByte, &Impl::cbExternalReadByte,
        &Impl::cbExternalWriteByte,
        &Impl::cbHandleTimer, &Impl::cbHandleIRQ, &impl->cbssg);
    ym2608_reset_chip(chip);
    ym2608_write(chip, 0, 0x29);
    ym2608_write(chip, 1, 0x9f);
}

void MameOPNA::reset()
{
    OPNChipBaseBufferedT::reset();
    void *chip = impl->chip;
    ym2608_reset_chip(chip);
    ym2608_write(chip, 0, 0x29);
    ym2608_write(chip, 1, 0x9f);
}

void MameOPNA::writeReg(uint32_t port, uint16_t addr, uint8_t data)
{
    void *chip = impl->chip;
    ym2608_write(chip, 0 + (int)(port) * 2, (uint8_t)addr);
    ym2608_write(chip, 1 + (int)(port) * 2, data);
}

void MameOPNA::writePan(uint16_t chan, uint8_t data)
{
    void *chip = impl->chip;
    ym2608_write_pan(chip, (int)chan, data);
}

void MameOPNA::nativeGenerateN(int16_t *output, size_t frames)
{
    void *chip = impl->chip;

    FMSAMPLE bufLR[2 * buffer_size];
    FMSAMPLE *bufR = bufLR + buffer_size;
    FMSAMPLE *bufs[2] = { bufLR, bufR };

    ym2608_update_one(chip, bufs, (int)frames);

    for(size_t i = 0; i < frames; ++i)
    {
        output[2 * i] = bufLR[i];
        output[2 * i + 1] = bufR[i];
    }
}

const char *MameOPNA::emulatorName()
{
    return "MAME YM2608";  // git 2018-12-15 rev 8ab05c0
}
