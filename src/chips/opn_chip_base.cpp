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
