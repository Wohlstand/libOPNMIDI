#ifndef NUKED_OPN2_H
#define NUKED_OPN2_H

#include "opn_chip_base.h"

class NukedOPN2 final : public OPNChipBase
{
    void *chip;
public:
    NukedOPN2();
    NukedOPN2(const NukedOPN2 &c);
    ~NukedOPN2() override;

    void setRate(uint32_t rate, uint32_t clock) override;
    void reset() override;
    void reset(uint32_t rate, uint32_t clock) override;
    void writeReg(uint32_t port, uint16_t addr, uint8_t data) override;
    int generate(int16_t *output, size_t frames) override;
    int generateAndMix(int16_t *output, size_t frames) override;
    int generate32(int32_t *output, size_t frames) override;
    int generateAndMix32(int32_t *output, size_t frames) override;
    const char *emulatorName() override;
};

#endif // NUKED_OPN2_H
