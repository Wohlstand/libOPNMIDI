#include "nuked_opn2.h"
#include "nuked/ym3438.h"
#include <cstring>

NukedOPN2::NukedOPN2() :
    OPNChipBase()
{
    OPN2_SetChipType(ym3438_type_asic);
    chip = new ym3438_t;
}

NukedOPN2::NukedOPN2(const NukedOPN2 &c):
    OPNChipBase(c)
{
    chip = new ym3438_t;
    std::memset(chip, 0, sizeof(ym3438_t));
    reset(c.m_rate, c.m_clock);
}

NukedOPN2::~NukedOPN2()
{
    ym3438_t *chip_r = reinterpret_cast<ym3438_t*>(chip);
    delete chip_r;
}

void NukedOPN2::setRate(uint32_t rate, uint32_t clock)
{
    OPNChipBase::setRate(rate, clock);
    ym3438_t *chip_r = reinterpret_cast<ym3438_t*>(chip);
    OPN2_Reset(chip_r, rate, clock);
}

void NukedOPN2::reset()
{
    setRate(m_rate, m_clock);
}

void NukedOPN2::reset(uint32_t rate, uint32_t clock)
{
    setRate(rate, clock);
}

void NukedOPN2::writeReg(uint32_t port, uint16_t addr, uint8_t data)
{
    ym3438_t *chip_r = reinterpret_cast<ym3438_t*>(chip);
    OPN2_WriteBuffered(chip_r, 0 + (port) * 2, (uint8_t)addr);
    OPN2_WriteBuffered(chip_r, 1 + (port) * 2, data);
    //qDebug() << QString("%1: 0x%2 => 0x%3").arg(port).arg(addr, 2, 16, QChar('0')).arg(data, 2, 16, QChar('0'));
}

int NukedOPN2::generate(int16_t *output, size_t frames)
{
    ym3438_t *chip_r = reinterpret_cast<ym3438_t*>(chip);
    OPN2_GenerateStream(chip_r, output, (Bit32u)frames);
    return (int)frames;
}

int NukedOPN2::generateAndMix(int16_t *output, size_t frames)
{
    ym3438_t *chip_r = reinterpret_cast<ym3438_t*>(chip);
    OPN2_GenerateStreamMix(chip_r, output, (Bit32u)frames);
    return (int)frames;
}

const char *NukedOPN2::emulatorName()
{
    return "Nuked OPN2";
}
