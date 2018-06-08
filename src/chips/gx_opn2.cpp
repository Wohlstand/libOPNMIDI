#include "gx_opn2.h"
#include <cstring>

#include "gx/ym2612.h"

GXOPN2::GXOPN2()
{
    chip = YM2612GXAlloc();
    YM2612GXInit(chip);
    YM2612GXConfig(chip, YM2612_ENHANCED);
}

GXOPN2::~GXOPN2()
{
    YM2612GXFree(chip);
}

void GXOPN2::setRate(uint32_t rate, uint32_t clock)
{
    OPNChipBaseBufferedT::setRate(rate, clock);
    YM2612GXResetChip(chip);
}

void GXOPN2::reset()
{
    OPNChipBaseBufferedT::reset();
    YM2612GXResetChip(chip);
}

void GXOPN2::writeReg(uint32_t port, uint16_t addr, uint8_t data)
{
    YM2612GXWrite(chip, 0 + port * 2, addr);
    YM2612GXWrite(chip, 1 + port * 2, data);
}

void GXOPN2::nativeGenerateN(int16_t *output, size_t frames)
{
    YM2612GXUpdate(chip, output, frames);
}

const char *GXOPN2::emulatorName()
{
    return "Genesis Plus GX";
}
