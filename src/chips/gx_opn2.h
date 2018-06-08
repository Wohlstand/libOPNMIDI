#ifndef GX_OPN2_H
#define GX_OPN2_H

#include "opn_chip_base.h"

struct YM2612GX;
class GXOPN2 final : public OPNChipBaseBufferedT<GXOPN2>
{
    YM2612GX *chip;
public:
    GXOPN2();
    ~GXOPN2() override;

    void setRate(uint32_t rate, uint32_t clock) override;
    void reset() override;
    void writeReg(uint32_t port, uint16_t addr, uint8_t data) override;
    void nativePreGenerate() override {}
    void nativePostGenerate() override {}
    void nativeGenerateN(int16_t *output, size_t frames) override;
    const char *emulatorName() override;
};

#endif // GX_OPN2_H
