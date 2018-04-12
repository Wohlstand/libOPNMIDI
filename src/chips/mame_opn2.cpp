#include "mame_opn2.h"
#include "mame/mame_ym2612fm.h"
#include <cstdlib>
#include <assert.h>

MameOPN2::MameOPN2() :
    OPNChipBase()
{
    chip = ym2612_init(NULL, (int)m_clock, (int)m_rate, NULL, NULL);
}

MameOPN2::MameOPN2(const MameOPN2 &c) :
    OPNChipBase(c)
{
    chip = ym2612_init(NULL, (int)m_clock, (int)m_rate, NULL, NULL);
}

MameOPN2::~MameOPN2()
{
    ym2612_shutdown(chip);
}

void MameOPN2::setRate(uint32_t rate, uint32_t clock)
{
    OPNChipBase::setRate(rate, clock);
    ym2612_shutdown(chip);
    chip = ym2612_init(NULL, (int)clock, (int)rate, NULL, NULL);
    ym2612_reset_chip(chip);
}

void MameOPN2::reset()
{
    ym2612_reset_chip(chip);
}

void MameOPN2::reset(uint32_t rate, uint32_t clock)
{
    setRate(rate, clock);
}

void MameOPN2::writeReg(uint32_t port, uint16_t addr, uint8_t data)
{
    ym2612_write(chip, 0 + (int)(port) * 2, (uint8_t)addr);
    ym2612_write(chip, 1 + (int)(port) * 2, data);
}

int MameOPN2::generate(int16_t *output, size_t frames)
{
    ym2612_generate(chip, output, (int)frames, 0);
    return (int)frames;
}

int MameOPN2::generateAndMix(int16_t *output, size_t frames)
{
    ym2612_generate(chip, output, (int)frames, 1);
    return (int)frames;
}

const char *MameOPN2::emulatorName()
{
    return "MAME YM2612";
}
