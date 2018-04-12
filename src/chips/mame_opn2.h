#ifndef MAME_OPN2_H
#define MAME_OPN2_H

#include "opn_chip_base.h"

class MameOPN2 final : public OPNChipBase
{
    void *chip;
public:
    MameOPN2();
    MameOPN2(const MameOPN2 &c);
    virtual ~MameOPN2() override;

    virtual void setRate(uint32_t rate, uint32_t clock) override;
    virtual void reset() override;
    virtual void reset(uint32_t rate, uint32_t clock) override;
    virtual void writeReg(uint32_t port, uint16_t addr, uint8_t data) override;
    virtual int generate(int16_t *output, size_t frames) override;
    virtual int generateAndMix(int16_t *output, size_t frames) override;
    virtual const char *emulatorName() override;
};

#endif // MAME_OPN2_H
