#ifndef GENS_OPN2_H
#define GENS_OPN2_H

#include "opn_chip_base.h"

class Ym2612_Emu;
class GensOPN2 final : public OPNChipBase
{
    Ym2612_Emu *chip;
public:
    GensOPN2();
    GensOPN2(const GensOPN2 &c);
    ~GensOPN2() override;

    void setRate(uint32_t rate, uint32_t clock) override;
    void reset() override;
    void reset(uint32_t rate, uint32_t clock) override;
    void writeReg(uint32_t port, uint16_t addr, uint8_t data) override;
    int generate(int16_t *output, size_t frames) override;
    int generateAndMix(int16_t *output, size_t frames) override;
    const char *emulatorName() override;
};

#endif // GENS_OPN2_H
