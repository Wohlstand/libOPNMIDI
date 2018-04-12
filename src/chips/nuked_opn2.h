#ifndef NUKED_OPN2_H
#define NUKED_OPN2_H

#include "opn_chip_base.h"

class NukedOPN2 final : public OPNChipBase
{
    void *chip;
public:
    NukedOPN2();
    NukedOPN2(const NukedOPN2 &c);
    virtual ~NukedOPN2() override;

    virtual void setRate(uint32_t rate, uint32_t clock) override;
    virtual void reset() override;
    virtual void reset(uint32_t rate, uint32_t clock) override;
    virtual void writeReg(uint32_t port, uint16_t addr, uint8_t data) override;
    virtual int generate(int16_t *output, size_t frames) override;
    virtual int generateAndMix(int16_t *output, size_t frames) override;
    virtual int generate32(int32_t *output, size_t frames) override;
    virtual int generateAndMix32(int32_t *output, size_t frames) override;
    virtual const char *emulatorName() override;
};

#endif // NUKED_OPN2_H
