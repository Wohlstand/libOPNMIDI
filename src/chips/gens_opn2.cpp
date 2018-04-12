#include "gens_opn2.h"
#include <cstring>

#include "gens/Ym2612_Emu.h"

GensOPN2::GensOPN2() : OPNChipBase(),
    chip(new Ym2612_Emu())
{}

GensOPN2::GensOPN2(const GensOPN2 &c) :
    OPNChipBase(c),
    chip(new Ym2612_Emu())
{
    reset(m_rate, m_clock);
}

GensOPN2::~GensOPN2()
{
    delete chip;
}

void GensOPN2::setRate(uint32_t rate, uint32_t clock)
{
    OPNChipBase::setRate(rate, clock);
    chip->set_rate(rate, clock);
}

void GensOPN2::reset()
{
    chip->reset();
}

void GensOPN2::reset(uint32_t rate, uint32_t clock)
{
    chip->set_rate(rate, clock);
    chip->reset();
}

void GensOPN2::writeReg(uint32_t port, uint16_t addr, uint8_t data)
{
    switch (port)
    {
    case 0:
        chip->write0(addr, data);
        break;
    case 1:
        chip->write1(addr, data);
        break;
    }
}

int GensOPN2::generate(int16_t *output, size_t frames)
{
    std::memset(output, 0, frames * sizeof(int16_t) * 2);
    chip->run((int)frames, output);
    return (int)frames;
}

int GensOPN2::generateAndMix(int16_t *output, size_t frames)
{
    chip->run((int)frames, output);
    return (int)frames;
}

const char *GensOPN2::emulatorName()
{
    return "GENS 2.10 OPN2";
}
