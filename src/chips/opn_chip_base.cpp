#include "opn_chip_base.h"

OPNChipBase::OPNChipBase() :
    m_rate(44100),
    m_clock(7670454)
{}

OPNChipBase::OPNChipBase(const OPNChipBase &c):
    m_rate(c.m_rate),
    m_clock(c.m_clock)
{}

OPNChipBase::~OPNChipBase()
{}

void OPNChipBase::setRate(uint32_t rate, uint32_t clock)
{
    m_rate = rate;
    m_clock = clock;
}

void OPNChipBase::reset(uint32_t rate, uint32_t clock)
{
    setRate(rate, clock);
}

int OPNChipBase::generate32(int32_t *output, size_t frames)
{
    enum { maxFramesAtOnce = 256 };
    int16_t temp[2 * maxFramesAtOnce];
    for(size_t left = frames; left > 0;) {
        size_t count = (left < maxFramesAtOnce) ? left : maxFramesAtOnce;
        int16_t *temp_it = temp;
        generate(temp, count);
        for(size_t i = 0; i < 2 * count; ++i)
            *(output++) = *(temp_it++);
        left -= count;
    }
    return (int)frames;
}

int OPNChipBase::generateAndMix32(int32_t *output, size_t frames)
{
    enum { maxFramesAtOnce = 256 };
    int16_t temp[2 * maxFramesAtOnce];
    for(size_t left = frames; left > 0;) {
        size_t count = (left < maxFramesAtOnce) ? left : maxFramesAtOnce;
        int16_t *temp_it = temp;
        generate(temp, count);
        for(size_t i = 0; i < 2 * count; ++i)
            *(output++) += *(temp_it++);
        left -= count;
    }
    return (int)frames;
}
