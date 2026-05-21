/*
 * OPNMIDI Player is a free MIDI player based on a libOPNMIDI,
 * a Software MIDI synthesizer library with OPN2 and OPNA emulation
 *
 * Original ADLMIDI code: Copyright (c) 2010-2014 Joel Yliluoma <bisqwit@iki.fi>
 * ADLMIDI Library API:   Copyright (c) 2015-2026 Vitaly Novichkov <admin@wohlnet.ru>
 *
 * Library is based on the ADLMIDI, a MIDI player for Linux and Windows with OPL3 emulation:
 * http://iki.fi/bisqwit/source/adlmidi.html
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

#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <cctype>
#include <cstring>
#include "opnmidi.h"
#include "misc.h"
#include "dev_setup.h"

Args                s_devSetup;

OPNMIDI_AudioFormat g_audioFormat = {OPNMIDI_SampleType_S16, 0 ,0};
float               g_gaining = 2.0f;

#define DEFAULT_BANK_NAME "xg.wopn"
#ifndef DEFAULT_INSTALL_PREFIX
#define DEFAULT_INSTALL_PREFIX "/usr"
#endif

static std::string findDefaultBank()
{
    const char *const paths[] =
        {
            DEFAULT_BANK_NAME,
            "../fm_banks/" DEFAULT_BANK_NAME,
#ifdef __unix__
            DEFAULT_INSTALL_PREFIX "/share/sounds/wopn/" DEFAULT_BANK_NAME,
            DEFAULT_INSTALL_PREFIX "/share/opnmidiplay/" DEFAULT_BANK_NAME,
#endif
            "../share/sounds/wopn/" DEFAULT_BANK_NAME,
            "../share/opnmidiplay/" DEFAULT_BANK_NAME,
        };
    const size_t paths_count = sizeof(paths) / sizeof(const char *);
    std::string ret;

    for(size_t i = 0; i < paths_count; i++)
    {
        const char *p = paths[i];
        FILE *probe = std::fopen(p, "rb");
        if(probe)
        {
            std::fclose(probe);
            ret = std::string(p);
            break;
        }
    }

    return ret;
}

static bool is_number(const std::string &s)
{
    std::string::const_iterator it = s.begin();
    while(it != s.end() && std::isdigit(*it)) ++it;
    return !s.empty() && it == s.end();
}

void printError(const char *err, const char *what)
{
    if(what)
        s_fprintf(stderr, "\nERROR (%s): %s\n\n", what, err);
    else
        s_fprintf(stderr, "\nERROR: %s\n\n", err);
    flushout(stderr);
}

Args::Args() :
    sampleRate(44100),
#if !defined(OUTPUT_WAVE_ONLY)
    recordWave(false),
#endif
    scaleModulators(false),
    fullRangedBrightness(false),
    loopEnabled(1),
    autoArpeggioEnabled(0),
    chanAlloc(OPNMIDI_ChanAlloc_AUTO),
    fullPanEnabled(false),
    emulator(OPNMIDI_EMU_MAME),
    volumeModel(OPNMIDI_VolumeModel_AUTO),
    soloTrack(~static_cast<size_t>(0u)),
    songNumLoad(-1),
    chipsCount(-1)

{
    spec.freq     = sampleRate;
    spec.format   = OPNMIDI_SampleType_S16;
    spec.channels = 2;
    spec.samples  = 1024; // uint16_t(static_cast<double>(spec.freq) * AudioBufferLength);
    spec.is_msb   = audio_is_big_endian();
}

int Args::parseArgs(int argc, char **argv_arr, bool *quit)
{
    int arg = 1;
    const char* const* argv = argv_arr;

    if(argc < 2 || std::string(argv[1]) == "--help" || std::string(argv[1]) == "-h")
    {
        std::printf(
            "Usage:\n"
            "   opnmidiplay [-s] [-w] [-nl] [--emu-mame|--emu-nuked|--emu-gens|--emu-gx|--emu-np2|--emu-mame-opna|--emu-pmdwin] \\\n"
            "               [--chips <count>] [<bankfile>.wopn] <midifilename>\n"
            "\n"
            " <bankfile>.wopn   Path to WOPN bank file\n"
            " <midifilename>    Path to music file to play\n"
            "\n"
            " -s                Enables scaling of modulator volumes\n"
            " -vm <num> Chooses one of volume models: \n"
            "    0 auto (default)\n"
            "    1 Generic\n"
            "    2 Native OPN2\n"
            "    3 DMX\n"
            "    4 Apogee Sound System\n"
            "    5 9x\n"
            " -ca <num> Chooses one of chip channel allocation modes: \n"
            "    0 Sounding delay"
            "    1 Released channel with the same instrument"
            "    2 Any released channel"
            " -frb              Enables full-ranged CC74 XG Brightness controller\n"
            " -mc <nums>        Mute selected MIDI channels"
            "                     where <num> - space separated numbers list (0-based!):"
            "                     Example: \"-mc 2 5 6 will\" mute channels 2, 5 and 6.\n"
            " --song <song ID 0...N-1> Selects a song to play (if XMI)\n"
            " -nl               Quit without looping\n"
#if !defined(OUTPUT_WAVE_ONLY)
            " -w                Write WAV file rather than playing\n"
#endif
            " -fp               Enables full-panning stereo support\n"
            " -ea               Enable the auto-arpeggio\n"
            " --gain <value>    Set the gaining factor (default 2.0)\n"
#ifndef OPNMIDI_DISABLE_MAME_EMULATOR
            " --emu-mame        Use MAME YM2612 Emulator\n"
#endif
#ifndef OPNMIDI_DISABLE_GENS_EMULATOR
            " --emu-gens        Use GENS 2.10 Emulator\n"
#endif
#ifndef OPNMIDI_DISABLE_NUKED_EMULATOR
            " --emu-nuked-3438  Use Nuked OPN2 YM3438 Emulator\n"
            " --emu-nuked-2612  Use Nuked OPN2 YM2612 Emulator\n"
            " --emu-nuked       Same as --emu-nuked-3438\n"
#endif
#ifndef OPNMIDI_DISABLE_YMFM_EMULATOR
            " --emu-ymfm-opn2   Use the YMFM OPN2\n"
#endif
// #ifndef OPNMIDI_DISABLE_GX_EMULATOR
// " --emu-gx          Use Genesis Plus GX Emulator\n"
// #endif
#ifndef OPNMIDI_DISABLE_NP2_EMULATOR
            " --emu-np2         Use Neko Project II Emulator\n"
#endif
#ifndef OPNMIDI_DISABLE_MAME_2608_EMULATOR
            " --emu-mame-opna   Use MAME YM2608 Emulator\n"
#endif
#ifndef OPNMIDI_DISABLE_YMFM_EMULATOR
            " --emu-ymfm-opna   Use the YMFM OPNA\n"
#endif
// #ifndef OPNMIDI_DISABLE_PMDWIN_EMULATOR
// " --emu-pmdwin      Use PMDWin Emulator\n"
// #endif
#ifdef OPNMIDI_ENABLE_OPNA_LLE_EMULATOR
            " --emu-lle-opna    Use the very accurate Nuked LLE YM2608 [EXTRA HEAVY]\n"
#endif
#ifdef OPNMIDI_ENABLE_OPNA_LLE_EMULATOR
            " --emu-lle-opn2    Use the very accurate Nuked LLE YM2612 [EXTRA HEAVY]\n"
            " --emu-lle-3438    Use the very accurate Nuked LLE YM3438 [EXTRA HEAVY]\n"
            " --emu-lle-f276    Use the very accurate Nuked LLE YMF276 [EXTRA HEAVY]\n"
#endif
            " --chips <count>   Choose a count of emulated concurrent chips\n"
            "\n"
            );
        std::fflush(stdout);

        *quit = true;
        return 0;
    }

    for(arg = 1; arg < argc; arg++)
    {
        if(!std::strcmp("-frb", argv[arg]))
            fullRangedBrightness = true;
#if !defined(OUTPUT_WAVE_ONLY)
        else if(!std::strcmp("-w", argv[arg]))
            recordWave = true;//Record library output into WAV file
        else if(!std::strcmp("-s8", argv[arg]) && !recordWave)
            spec.format = OPNMIDI_SampleType_S8;
        else if(!std::strcmp("-u8", argv[arg]) && !recordWave)
            spec.format = OPNMIDI_SampleType_U8;
        else if(!std::strcmp("-s16", argv[arg]) && !recordWave)
            spec.format = OPNMIDI_SampleType_S16;
        else if(!std::strcmp("-u16", argv[arg]) && !recordWave)
            spec.format = OPNMIDI_SampleType_U16;
        else if(!std::strcmp("-s32", argv[arg]) && !recordWave)
            spec.format = OPNMIDI_SampleType_S32;
        else if(!std::strcmp("-f32", argv[arg]) && !recordWave)
            spec.format = OPNMIDI_SampleType_F32;
#endif
        else if(!std::strcmp("-nl", argv[arg]))
            loopEnabled = 0; //Enable loop
        else if(!std::strcmp("-na", argv[arg]))
            autoArpeggioEnabled = 0;
        else if(!std::strcmp("-ea", argv[arg]))
            autoArpeggioEnabled = 1; //Enable automatical arpeggio
        else if(!std::strcmp("--emu-nuked", argv[arg]))
            emulator = OPNMIDI_EMU_NUKED_YM3438;
        else if(!std::strcmp("--emu-nuked-3438", argv[arg]))
            emulator = OPNMIDI_EMU_NUKED_YM3438;
        else if(!std::strcmp("--emu-nuked-2612", argv[arg]))
            emulator = OPNMIDI_EMU_NUKED_YM2612;
        else if(!std::strcmp("--emu-lle-2608", argv[arg]) || !std::strcmp("--emu-lle-opna", argv[arg]))
            emulator = OPNMIDI_EMU_NUKED_YM2608_LLE;
        else if(!std::strcmp("--emu-lle-2612", argv[arg]) || !std::strcmp("--emu-lle-opn2", argv[arg]))
            emulator = OPNMIDI_EMU_NUKED_YM2612_LLE;
        else if(!std::strcmp("--emu-lle-3438", argv[arg]))
            emulator = OPNMIDI_EMU_NUKED_YM3438_LLE;
        else if(!std::strcmp("--emu-lle-f276", argv[arg]))
            emulator = OPNMIDI_EMU_NUKED_YMF276_LLE;
        else if(!std::strcmp("--emu-gens", argv[arg]))
            emulator = OPNMIDI_EMU_GENS;
        else if(!std::strcmp("--emu-mame", argv[arg]))
            emulator = OPNMIDI_EMU_MAME;
        else if(!std::strcmp("--emu-gx", argv[arg]))
        {
            std::fprintf(stdout, " - WARNING: GX is deprecated. Using YMFM OPN2 instead.\n");
            std::fflush(stdout);
            emulator = OPNMIDI_EMU_YMFM_OPN2;
        }
        else if(!std::strcmp("--emu-ymfm-opn2", argv[arg]))
            emulator = OPNMIDI_EMU_YMFM_OPN2;
        else if(!std::strcmp("--emu-np2", argv[arg]))
            emulator = OPNMIDI_EMU_NP2;
        else if(!std::strcmp("--emu-mame-opna", argv[arg]))
            emulator = OPNMIDI_EMU_MAME_2608;
        else if(!std::strcmp("--emu-pmdwin", argv[arg])) // DEPRECATED FLAG
        {
            std::fprintf(stdout, " - WARNING: PMDWin is deprecated. Using YMFM OPNA instead.\n");
            std::fflush(stdout);
            emulator = OPNMIDI_EMU_YMFM_OPNA;
        }
        else if(!std::strcmp("--emu-ymfm-opna", argv[arg]))
            emulator = OPNMIDI_EMU_YMFM_OPNA;
        else if(!std::strcmp("-fp", argv[arg]))
            fullPanEnabled = true;
        else if(!std::strcmp("-s", argv[arg]))
            scaleModulators = true;
        else if(!std::strcmp("--gain", argv[arg]))
        {
            if(arg + 1 >= argc)
            {
                printError("The option --gain requires an argument!\n");
                *quit = true;
                return 1;
            }
            g_gaining = std::atof(argv[++arg]);
        }
        else if(!std::strcmp("-vm", argv[arg]))
        {
            if(arg + 1 >= argc)
            {
                printError("The option -vm requires an argument!\n");
                *quit = true;
                return 1;
            }
            volumeModel = static_cast<int>(std::strtol(argv[++arg], NULL, 10));
        }
        else if(!std::strcmp("-ca", argv[arg]))
        {
            if(arg + 1 >= argc)
            {
                printError("The option -carequires an argument!\n");
                *quit = true;
                return 1;
            }
            chanAlloc = static_cast<int>(std::strtol(argv[++arg], NULL, 10));
        }
        else if(!std::strcmp("--chips", argv[arg]))
        {
            if(arg + 1 >= argc)
            {
                printError("The option --chips requires an argument!\n");
                *quit = true;
                return 1;
            }
            chipsCount = static_cast<int>(std::strtoul(argv[++arg], NULL, 10));
        }
        else if(!std::strcmp("--solo", argv[arg]))
        {
            if(arg + 1 >= argc)
            {
                printError("The option --solo requires an argument!\n");
                *quit = true;
                return 1;
            }
            soloTrack = std::strtoul(argv[++arg], NULL, 10);
        }
        else if(!std::strcmp("--song", argv[arg]))
        {
            if(argc <= 3)
            {
                printError("The option --song requires an argument!\n");
                *quit = true;
                return 1;
            }
            songNumLoad = std::strtol(argv[++arg], NULL, 10);
        }
        else if(!std::strcmp("-mc", argv[arg]) || !std::strcmp("--mute-channels", argv[arg]))
        {
            if(arg + 1 >= argc)
            {
                printError("The option -mc/--mute-channels requires an argument!\n");
                *quit = true;
                return 1;
            }

            while(arg + 1 < argc && is_number(argv[arg + 1]))
            {
                int num = static_cast<int>(std::strtoul(argv[++arg], NULL, 0));
                if(num >= 0 && num <= 15)
                    muteChannels.push_back(num);
            }
        }
        else if(!std::strcmp("--", argv[arg]))
            break;
        else
            break;
    }

    if(arg == argc - 2)
    {
        bankPath = argv[arg];
        musPath = argv[arg + 1];
    }
    else if(arg == argc - 1)
    {
        std::fprintf(stdout, " - Bank is not specified, searching for default...\n");
        std::fflush(stdout);
        bankPath = findDefaultBank();
        if(bankPath.empty())
        {
            printError("Missing default bank file xg.wopn!\n");
            return 2;
        }
        musPath = argv[arg];
    }
    else if(arg > argc - 1)
    {
        printError("Missing music file path!\n");
        return 2;
    }

    *quit = false;

    return 0;
}
