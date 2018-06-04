#ifndef MAME_OPN2_H
#define MAME_OPN2_H

#include "opn_chip_base.h"

#if defined(OPNMIDI_ENABLE_HQ_RESAMPLER)
class VResampler;
#endif

class MameOPN2 final : public OPNChipBase
{
    void *chip;
#if defined(OPNMIDI_ENABLE_HQ_RESAMPLER)
    VResampler *m_resampler;
#endif
public:
    MameOPN2();
    ~MameOPN2() override;

    void setRate(uint32_t rate, uint32_t clock) override;
    void reset() override;
    void reset(uint32_t rate, uint32_t clock) override;
    void writeReg(uint32_t port, uint16_t addr, uint8_t data) override;
    int generate(int16_t *output, size_t frames) override;
    int generateAndMix(int16_t *output, size_t frames) override;
    const char *emulatorName() override;
private:
#if defined(OPNMIDI_ENABLE_HQ_RESAMPLER)
    void generateResampledHq(int16_t *out);
    void generateResampledHq32(int32_t *out);
#endif
};

#endif // MAME_OPN2_H
