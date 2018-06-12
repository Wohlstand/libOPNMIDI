#ifndef GX_OPN2_H
#define GX_OPN2_H

#include "opn_chip_base.h"

struct YM2612GX;
class GXOPN2 final : public OPNChipBaseT<GXOPN2>
{
    YM2612GX *m_chip;
    unsigned int m_framecount;
public:
    GXOPN2();
    ~GXOPN2() override;

    bool canRunAtPcmRate() const override { return false; }
    void setRate(uint32_t rate, uint32_t clock) override;
    void reset() override;
    void writeReg(uint32_t port, uint16_t addr, uint8_t data) override;
    void nativePreGenerate() override;
    void nativePostGenerate() override;
    void nativeGenerate(int16_t *frame) override;
    const char *emulatorName() override;
};

#endif // GX_OPN2_H
