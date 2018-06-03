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
    virtual ~MameOPN2() override;

    virtual void setRate(uint32_t rate, uint32_t clock) override;
    virtual void reset() override;
    virtual void reset(uint32_t rate, uint32_t clock) override;
    virtual void writeReg(uint32_t port, uint16_t addr, uint8_t data) override;
    virtual int generate(int16_t *output, size_t frames) override;
    virtual int generateAndMix(int16_t *output, size_t frames) override;
    virtual const char *emulatorName() override;
private:
#if defined(OPNMIDI_ENABLE_HQ_RESAMPLER)
    void generateResampledHq(int16_t *out);
    void generateResampledHq32(int32_t *out);
#endif
};

#endif // MAME_OPN2_H
