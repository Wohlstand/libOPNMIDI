#include "opn_chip_base.h"

template <class T>
int OPNChipBaseT<T>::generate32(int32_t *output, size_t frames)
{
    enum { maxFramesAtOnce = 256 };
    int16_t temp[2 * maxFramesAtOnce];
    for(size_t left = frames; left > 0;) {
        size_t count = (left < (size_t)maxFramesAtOnce) ? left : (size_t)maxFramesAtOnce;
        int16_t *temp_it = temp;
        static_cast<T *>(this)->generate(temp, count);
        for(size_t i = 0; i < 2 * count; ++i)
            *(output++) = *(temp_it++);
        left -= count;
    }
    return (int)frames;
}

template <class T>
int OPNChipBaseT<T>::generateAndMix32(int32_t *output, size_t frames)
{
    enum { maxFramesAtOnce = 256 };
    int16_t temp[2 * maxFramesAtOnce];
    for(size_t left = frames; left > 0;) {
        size_t count = (left < (size_t)maxFramesAtOnce) ? left : (size_t)maxFramesAtOnce;
        int16_t *temp_it = temp;
        static_cast<T *>(this)->generate(temp, count);
        for(size_t i = 0; i < 2 * count; ++i)
            *(output++) += *(temp_it++);
        left -= count;
    }
    return (int)frames;
}
