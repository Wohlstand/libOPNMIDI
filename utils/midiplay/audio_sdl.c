/*
 * OPNMIDI Player is a free MIDI player based on a libOPNMIDI,
 * a Software MIDI synthesizer library with OPL3 emulation
 *
 * Original OPNMIDI code: Copyright (c) 2010-2014 Joel Yliluoma <bisqwit@iki.fi>
 * OPNMIDI Library API:   Copyright (c) 2015-2024 Vitaly Novichkov <admin@wohlnet.ru>
 *
 * Library is based on the OPNMIDI, a MIDI player for Linux and Windows with OPL3 emulation:
 * http://iki.fi/bisqwit/source/OPNMIDI.html
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <opnmidi.h>
#include "audio.h"
#define SDL_MAIN_HANDLED
#include <SDL2/SDL.h>

int audio_init(struct AudioOutputSpec *in_spec, struct AudioOutputSpec *out_obtained, AudioOutputCallback callback)
{
    SDL_AudioSpec spec;
    SDL_AudioSpec obtained;
    int ret;

    spec.format = AUDIO_S16SYS;
    spec.freq = (int)in_spec->freq;
    spec.samples = in_spec->samples;
    spec.channels = in_spec->channels;
    spec.callback = callback;

    switch(in_spec->format)
    {
    case OPNMIDI_SampleType_S8:
        spec.format = AUDIO_S8; break;
    case OPNMIDI_SampleType_U8:
        spec.format = AUDIO_U8; break;
    case OPNMIDI_SampleType_S16:
#if SDL_BYTEORDER == SDL_LIL_ENDIAN
        spec.format = AUDIO_S16;
#else
        spec.format = AUDIO_S16MSB;
#endif
        break;
    case OPNMIDI_SampleType_U16:
#if SDL_BYTEORDER == SDL_LIL_ENDIAN
        spec.format = AUDIO_U16;
#else
        spec.format = AUDIO_U16MSB;
#endif
        break;
    case OPNMIDI_SampleType_S32:
#if SDL_BYTEORDER == SDL_LIL_ENDIAN
        spec.format = AUDIO_S32;
#else
        spec.format = AUDIO_S32MSB;
#endif
        break;
    case OPNMIDI_SampleType_F32:
#if SDL_BYTEORDER == SDL_LIL_ENDIAN
        spec.format = AUDIO_F32;
#else
        spec.format = AUDIO_F32MSB;
#endif
        break;
    }

    ret = SDL_OpenAudio(&spec, &obtained);

    out_obtained->channels = obtained.channels;
    out_obtained->freq = (unsigned int)obtained.freq;
    out_obtained->samples = obtained.samples;
    out_obtained->format = in_spec->format;
    out_obtained->is_msb = 0;

    switch(obtained.format)
    {
    case AUDIO_S8:
        out_obtained->format = OPNMIDI_SampleType_S8; break;
    case AUDIO_U8:
        out_obtained->format = OPNMIDI_SampleType_U8; break;
    case AUDIO_S16MSB:
        out_obtained->is_msb = 1;/* fallthrough */
    case AUDIO_S16:
        out_obtained->format = OPNMIDI_SampleType_S16; break;
    case AUDIO_U16MSB:
        out_obtained->is_msb = 1;/* fallthrough */
    case AUDIO_U16:
        out_obtained->format = OPNMIDI_SampleType_U16; break;
    case AUDIO_S32MSB:
        out_obtained->is_msb = 1;/* fallthrough */
    case AUDIO_S32:
        out_obtained->format = OPNMIDI_SampleType_S32; break;
    case AUDIO_F32MSB:
        out_obtained->is_msb = 1;/* fallthrough */
    case AUDIO_F32:
        out_obtained->format = OPNMIDI_SampleType_F32; break;
    }

    return ret;
}

int audio_is_big_endian(void)
{
#if SDL_BYTEORDER == SDL_LIL_ENDIAN
    return 0;
#else
    return 1;
#endif
}

void audio_close(void)
{
    SDL_CloseAudio();
}

const char* audio_get_error(void)
{
    return SDL_GetError();
}

void audio_start(void)
{
    SDL_PauseAudio(0);
}

void audio_stop(void)
{
    SDL_PauseAudio(1);
}

void audio_lock(void)
{
    SDL_LockAudio();
}

void audio_unlock(void)
{
    SDL_UnlockAudio();
}

void audio_delay(unsigned int ms)
{
    SDL_Delay(ms);
}

void* audio_mutex_create(void)
{
    return SDL_CreateMutex();
}

void  audio_mutex_destroy(void *m)
{
    SDL_mutex *mut = (SDL_mutex *)m;
    SDL_DestroyMutex(mut);
}

void  audio_mutex_lock(void *m)
{
    SDL_mutex *mut = (SDL_mutex *)m;
    SDL_mutexP(mut);
}

void  audio_mutex_unlock(void *m)
{
    SDL_mutex *mut = (SDL_mutex *)m;
    SDL_mutexV(mut);
}
