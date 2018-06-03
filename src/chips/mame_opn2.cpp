#include "mame_opn2.h"
#include "mame/mame_ym2612fm.h"
#include <cstdlib>
#include <cmath>
#include <assert.h>

#if defined(OPNMIDI_ENABLE_HQ_RESAMPLER)
#include <zita-resampler/vresampler.h>
#endif

MameOPN2::MameOPN2() :
    OPNChipBase()
{
    chip = NULL;
#if defined(OPNMIDI_ENABLE_HQ_RESAMPLER)
    m_resampler = new VResampler;
#endif
    setRate(m_rate, m_clock);
}

MameOPN2::~MameOPN2()
{
    ym2612_shutdown(chip);
}

void MameOPN2::setRate(uint32_t rate, uint32_t clock)
{
    OPNChipBase::setRate(rate, clock);
    if(chip)
        ym2612_shutdown(chip);
    chip = ym2612_init(NULL, (int)clock, (int)rate, NULL, NULL);
    ym2612_reset_chip(chip);
#if defined(OPNMIDI_ENABLE_HQ_RESAMPLER)
    m_resampler->setup(rate * (1.0 / 53267), 2, 48);
#endif
}

void MameOPN2::reset()
{
    ym2612_reset_chip(chip);
}

void MameOPN2::reset(uint32_t rate, uint32_t clock)
{
    setRate(rate, clock);
}

void MameOPN2::writeReg(uint32_t port, uint16_t addr, uint8_t data)
{
    ym2612_write(chip, 0 + (int)(port) * 2, (uint8_t)addr);
    ym2612_write(chip, 1 + (int)(port) * 2, data);
}

int MameOPN2::generate(int16_t *output, size_t frames)
{
    void *chip = this->chip;
#if defined(OPNMIDI_ENABLE_HQ_RESAMPLER)
    ym2612_pre_generate(chip);
    for(size_t i = 0; i < frames; ++i)
    {
        generateResampledHq(output);
        output += 2;
    }
    // ym2612_post_generate(chip);
#else
    ym2612_generate(chip, output, (int)frames, 0);
#endif
    return (int)frames;
}

int MameOPN2::generateAndMix(int16_t *output, size_t frames)
{
    void *chip = this->chip;
#if defined(OPNMIDI_ENABLE_HQ_RESAMPLER)
    ym2612_pre_generate(chip);
    for(size_t i = 0; i < frames; ++i)
    {
        int32_t buf[2];
        generateResampledHq32(buf);
        for (unsigned c = 0; c < 2; ++c) {
            int32_t temp = (int32_t)output[c] + buf[c];
            temp = (temp > -32768) ? temp : -32768;
            temp = (temp < 32767) ? temp : 32767;
            output[c] = temp;
        }
        output += 2;
    }
    // ym2612_post_generate(chip);
#else
    ym2612_generate(chip, output, (int)frames, 1);
#endif
    return (int)frames;
}

#if defined(OPNMIDI_ENABLE_HQ_RESAMPLER)
void MameOPN2::generateResampledHq(int16_t *out)
{
    int32_t temps[2];
    generateResampledHq32(temps);
    for(unsigned i = 0; i < 2; ++i)
    {
        int32_t temp = temps[i];
        temp = (temp > -32768) ? temp : -32768;
        temp = (temp < 32767) ? temp : 32767;
        out[i] = temp;
    }
}

void MameOPN2::generateResampledHq32(int32_t *out)
{
    void *chip = this->chip;
    VResampler *rsm = m_resampler;
    float f_in[2];
    float f_out[2];
    rsm->inp_count = 0;
    rsm->inp_data = f_in;
    rsm->out_count = 1;
    rsm->out_data = f_out;
    while(rsm->process(), rsm->out_count != 0)
    {
        int16_t in[2];
        ym2612_generate_one_native(chip, in);
        f_in[0] = (float)in[0], f_in[1] = (float)in[1];
        rsm->inp_count = 1;
        rsm->inp_data = f_in;
        rsm->out_count = 1;
        rsm->out_data = f_out;
    }
    out[0] = std::lround(f_out[0]);
    out[1] = std::lround(f_out[1]);
}
#endif

const char *MameOPN2::emulatorName()
{
    return "MAME YM2612";
}
