/*
 * MIDI to VGM converter, an additional tool included with libOPNMIDI library
 *
 * Copyright (c) 2015-2025 Vitaly Novichkov <admin@wohlnet.ru>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
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

#include <vector>
#include <string>
#include <cstdio>
#include <cctype>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <cstdarg>
#include <deque>
#include <algorithm>
#include <signal.h>

#include "compact/vgm_cmp.h"

#if defined(_MSC_VER) && _MSC_VER < 1900

#define snprintf c99_snprintf
#define vsnprintf c99_vsnprintf

__inline int c99_vsnprintf(char *outBuf, size_t size, const char *format, va_list ap)
{
    int count = -1;

    if (size != 0)
        count = _vsnprintf_s(outBuf, size, _TRUNCATE, format, ap);
    if (count == -1)
        count = _vscprintf(format, ap);

    return count;
}

__inline int c99_snprintf(char *outBuf, size_t size, const char *format, ...)
{
    int count;
    va_list ap;

    va_start(ap, format);
    count = c99_vsnprintf(outBuf, size, format, ap);
    va_end(ap);

    return count;
}
#endif

#include <opnmidi.h>

extern "C"
{
    extern OPNMIDI_DECLSPEC void opn2_set_vgm_out_path(const char *path);
}

const char* volume_model_to_str(int vm)
{
    switch(vm)
    {
    default:
    case OPNMIDI_VolumeModel_Generic:
        return "Generic";
    case OPNMIDI_VolumeModel_NativeOPN2:
        return "Native OPN2";
    case OPNMIDI_VolumeModel_DMX:
        return "DMX";
    case OPNMIDI_VolumeModel_APOGEE:
        return "Apogee Sound System";
    case OPNMIDI_VolumeModel_9X:
        return "9X";
    }
}

const char* chanalloc_to_str(int vm)
{
    switch(vm)
    {
    default:
    case OPNMIDI_ChanAlloc_AUTO:
        return "<auto>";
    case OPNMIDI_ChanAlloc_OffDelay:
        return "Off Delay";
    case OPNMIDI_ChanAlloc_SameInst:
        return "Same instrument";
    case OPNMIDI_ChanAlloc_AnyReleased:
        return "Any released";
    }
}

static void printError(const char *err)
{
    std::fprintf(stderr, "\nERROR: %s\n\n", err);
    std::fflush(stderr);
}

static int stop = 0;
static void sighandler(int dum)
{
    if((dum == SIGINT)
        || (dum == SIGTERM)
    #ifndef _WIN32
        || (dum == SIGHUP)
    #endif
    )
        stop = 1;
}


static void debugPrint(void * /*userdata*/, const char *fmt, ...)
{
    char buffer[4096];
    std::va_list args;
    va_start(args, fmt);
    int rc = vsnprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);
    if(rc > 0)
    {
        std::fprintf(stdout, " - Debug: %s\n", buffer);
        std::fflush(stdout);
    }
}

#ifdef DEBUG_TRACE_ALL_EVENTS
static void debugPrintEvent(void * /*userdata*/, OPN2_UInt8 type, OPN2_UInt8 subtype, OPN2_UInt8 channel, const OPN2_UInt8 * /*data*/, size_t len)
{
    std::fprintf(stdout, " - E: 0x%02X 0x%02X %02d (%d)\r\n", type, subtype, channel, (int)len);
    std::fflush(stdout);
}
#endif

static inline void secondsToHMSM(double seconds_full, char *hmsm_buffer, size_t hmsm_buffer_size)
{
    double seconds_integral;
    double seconds_fractional = std::modf(seconds_full, &seconds_integral);
    unsigned int milliseconds = static_cast<unsigned int>(seconds_fractional * 1000.0);
    unsigned int seconds = static_cast<unsigned int>(std::fmod(seconds_full, 60.0));
    unsigned int minutes = static_cast<unsigned int>(std::fmod(seconds_full / 60, 60.0));
    unsigned int hours   = static_cast<unsigned int>(seconds_full / 3600);
    std::memset(hmsm_buffer, 0, hmsm_buffer_size);
    if (hours > 0)
        snprintf(hmsm_buffer, hmsm_buffer_size, "%02u:%02u:%02u,%03u", hours, minutes, seconds, milliseconds);
    else
        snprintf(hmsm_buffer, hmsm_buffer_size, "%02u:%02u,%03u", minutes, seconds, milliseconds);
}


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
    const size_t paths_count = sizeof(paths) / sizeof(const char*);
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
    while(it != s.end() && std::isdigit(*it))
        ++it;
    return !s.empty() && it == s.end();
}


int main(int argc, char **argv)
{
    std::fprintf(stdout, "==========================================================\n"
                         "             MIDI to VGM conversion utility\n"
                         "==========================================================\n"
                         "   Source code: https://github.com/Wohlstand/libOPNMIDI\n"
                         "==========================================================\n"
                         "\n");
    std::fflush(stdout);

    if(argc < 2 || std::string(argv[1]) == "--help" || std::string(argv[1]) == "-h")
    {
        std::printf(
            "Usage:\n"
            "   midi2vgm [-s] [-w] [-l] [-na] [-z] [-frb] \\\n"
            "               [--chips <count>] [<bankfile>.wopn] <midifilename>\n"
            "\n"
            " <bankfile>.wopn   Path to WOPN bank file\n"
            " <midifilename>    Path to music file to play\n"
            "\n"
            " -l                Enables in-song looping support\n"
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
            " -na               Disables the automatical arpeggio\n"
            " -z                Make a compressed VGZ file\n"
            " -frb              Enables full-ranged CC74 XG Brightness controller\n"
            " -mc <nums>        Mute selected MIDI channels"
            "                     where <num> - space separated numbers list (0-based!):"
            "                     Example: \"-mc 2 5 6 will\" mute channels 2, 5 and 6.\n"
            " --song <song ID 0...N-1> Selects a song to play (if XMI)\n"
            " --chips <count>   Choose a count of chips (1 by default, 2 maximum)\n"
            "\n"
            "----------------------------------------------------------\n"
            "TIP-1: Single-chip mode limits polyphony up to 6 channels.\n"
            "       Use Dual-Chip mode (with \"--chips 2\" flag) to extend\n"
            "       polyphony up to 12 channels!\n"
            "\n"
            "TIP-2: Use addional WOPN bank files to alter sounding of generated song.\n"
            "\n"
            "TIP-3: Use \"vgm_cmp\" tool from 'VGM Tools' toolchain to optimize size of\n"
            "       generated VGM song when it appears too bug (300+, 500+, or even 1000+ KB).\n"
            "\n"
        );
        std::fflush(stdout);

        return 0;
    }

    int sampleRate = 44100;

    OPN2_MIDIPlayer *myDevice;

    /*
     * Set library options by parsing of command line arguments
     */
    bool scaleModulators = false;
    bool makeVgz = false;
    bool fullRangedBrightness = false;
    int loopEnabled = 0;
    int volumeModel = OPNMIDI_VolumeModel_AUTO;
    int autoArpeggioEnabled = 0;
    int chanAlloc = OPNMIDI_ChanAlloc_AUTO;
    size_t soloTrack = ~static_cast<size_t>(0);
    int songNumLoad = -1;
    int chipsCount = 1;// Single-chip by default
    std::vector<int> muteChannels;

    std::string bankPath;
    std::string musPath;

    int arg = 1;
    for(arg = 1; arg < argc; arg++)
    {
        if(!std::strcmp("-frb", argv[arg]))
            fullRangedBrightness = true;
        else if(!std::strcmp("-s", argv[arg]))
            scaleModulators = true;
        else if(!std::strcmp("-z", argv[arg]))
            makeVgz = true;
        else if(!std::strcmp("-l", argv[arg]))
            loopEnabled = 1;
        else if(!std::strcmp("-na", argv[arg]))
            autoArpeggioEnabled = 0;
        else if(!std::strcmp("-ea", argv[arg]))
            autoArpeggioEnabled = 1; //Enable automatical arpeggio
        else if(!std::strcmp("-vm", argv[arg]))
        {
            if(arg + 1 >= argc)
            {
                printError("The option -vm requires an argument!\n");
                return 1;
            }
            volumeModel = static_cast<int>(std::strtol(argv[++arg], NULL, 10));
        }
        else if(!std::strcmp("-ca", argv[arg]))
        {
            if(arg + 1 >= argc)
            {
                printError("The option -carequires an argument!\n");
                return 1;
            }
            chanAlloc = static_cast<int>(std::strtol(argv[++arg], NULL, 10));
        }
        else if(!std::strcmp("--chips", argv[arg]))
        {
            if(arg + 1 >= argc)
            {
                printError("The option --chips requires an argument!\n");
                return 1;
            }
            chipsCount = static_cast<int>(std::strtoul(argv[++arg], NULL, 0));
        }
        else if(!std::strcmp("--solo", argv[arg]))
        {
            if(arg + 1 >= argc)
            {
                printError("The option --solo requires an argument!\n");
                return 1;
            }
            soloTrack = std::strtoul(argv[++arg], NULL, 10);
        }
        else if(!std::strcmp("--song", argv[arg]))
        {
            if(argc <= 3)
            {
                printError("The option --song requires an argument!\n");
                return 1;
            }
            songNumLoad = std::strtol(argv[++arg], NULL, 10);
        }
        else if(!std::strcmp("-mc", argv[arg]) || !std::strcmp("--mute-channels", argv[arg]))
        {
            if(arg + 1 >= argc)
            {
                printError("The option -mc/--mute-channels requires an argument!\n");
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

    std::string vgm_out = musPath + (makeVgz ? ".vgz" : ".vgm");
    opn2_set_vgm_out_path(vgm_out.c_str());

    myDevice = opn2_init(sampleRate);
    if(!myDevice)
    {
        std::fprintf(stderr, "Failed to init MIDI device!\n");
        return 1;
    }

    //Set internal debug messages hook to print all libADLMIDI's internal debug messages
    opn2_setDebugMessageHook(myDevice, debugPrint, NULL);

    //Turn loop on/off (for WAV recording loop must be disabled!)
    if(scaleModulators)
        opn2_setScaleModulators(myDevice, 1);//Turn on modulators scaling by volume
    if(fullRangedBrightness)
        opn2_setFullRangeBrightness(myDevice, 1);//Turn on a full-ranged XG CC74 Brightness
    opn2_setSoftPanEnabled(myDevice, 0);
    opn2_setLoopEnabled(myDevice, loopEnabled);
    opn2_setAutoArpeggio(myDevice, autoArpeggioEnabled);
    opn2_setChannelAllocMode(myDevice, chanAlloc);

#ifdef DEBUG_TRACE_ALL_EVENTS
    //Hook all MIDI events are ticking while generating an output buffer
    if(!recordWave)
        opn2_setRawEventHook(myDevice, debugPrintEvent, NULL);
#endif

    if(songNumLoad >= 0)
        opn2_selectSongNum(myDevice, songNumLoad);

    if(opn2_switchEmulator(myDevice, OPNMIDI_VGM_DUMPER) != 0)
    {
        std::fprintf(stdout, "FAILED!\n");
        std::fflush(stdout);
        printError(opn2_errorInfo(myDevice));
        return 2;
    }

    std::fprintf(stdout, " - Library version %s\n", opn2_linkedLibraryVersion());
    std::fprintf(stdout, " - %s Emulator in use\n", opn2_chipEmulatorName(myDevice));

    std::fprintf(stdout, " - Use bank [%s]...", bankPath.c_str());
    std::fflush(stdout);

    if(opn2_openBankFile(myDevice, bankPath.c_str()) != 0)
    {
        std::fprintf(stdout, "FAILED!\n");
        std::fflush(stdout);
        printError(opn2_errorInfo(myDevice));
        return 2;
    }
    std::fprintf(stdout, "OK!\n");

    if(chipsCount < 0)
        chipsCount = 1;
    else if(chipsCount > 2)
        chipsCount = 2;
    opn2_setNumChips(myDevice, chipsCount);

    if(volumeModel != OPNMIDI_VolumeModel_AUTO)
        opn2_setVolumeRangeModel(myDevice, volumeModel);

    if(opn2_openFile(myDevice, musPath.c_str()) != 0)
    {
        printError(opn2_errorInfo(myDevice));
        return 2;
    }

    std::fprintf(stdout, " - Number of chips %d\n", opn2_getNumChipsObtained(myDevice));
    std::fprintf(stdout, " - Track count: %lu\n", (unsigned long)opn2_trackCount(myDevice));
    std::fprintf(stdout, " - Volume model: %s\n", volume_model_to_str(opn2_getVolumeRangeModel(myDevice)));
    std::fprintf(stdout, " - Channel allocation mode: %s\n", chanalloc_to_str(opn2_getChannelAllocMode(myDevice)));

    int songsCount = opn2_getSongsCount(myDevice);
    if(songNumLoad >= 0)
        std::fprintf(stdout, " - Attempt to load song number: %d / %d\n", songNumLoad, songsCount);
    else if(songsCount > 0)
        std::fprintf(stdout, " - File contains %d song(s)\n", songsCount);

    if(soloTrack != ~(size_t)0)
    {
        std::fprintf(stdout, " - Solo track: %lu\n", (unsigned long)soloTrack);
        opn2_setTrackOptions(myDevice, soloTrack, OPNMIDI_TrackOption_Solo);
    }

    if(!muteChannels.empty())
    {
        std::printf(" - Mute MIDI channels:");

        for(size_t i = 0; i < muteChannels.size(); ++i)
        {
            opn2_setChannelEnabled(myDevice, muteChannels[i], 0);
            std::printf(" %d", muteChannels[i]);
        }

        std::printf("\n");
        std::fflush(stdout);
    }

    std::fprintf(stdout, " - Automatic arpeggion is turned %s\n", opn2_getAutoArpeggio(myDevice) ? "ON" : "OFF");

    std::fprintf(stdout, " - File [%s] opened!\n", musPath.c_str());
    std::fflush(stdout);

    signal(SIGINT, sighandler);
    signal(SIGTERM, sighandler);
    #ifndef _WIN32
    signal(SIGHUP, sighandler);
    #endif

    double total        = opn2_totalTimeLength(myDevice);
    double loopStart    = opn2_loopStartTime(myDevice);
    double loopEnd      = opn2_loopEndTime(myDevice);
    char totalHMS[25];
    char loopStartHMS[25];
    char loopEndHMS[25];
    secondsToHMSM(total, totalHMS, 25);
    if(loopStart >= 0.0 && loopEnd >= 0.0)
    {
        secondsToHMSM(loopStart, loopStartHMS, 25);
        secondsToHMSM(loopEnd, loopEndHMS, 25);
    }

    std::fprintf(stdout, " - Recording VGM file %s...\n", vgm_out.c_str());
    std::fprintf(stdout, "\n==========================================\n");
    std::fflush(stdout);

    short buff[4096];
    while(!stop)
    {
        size_t got = (size_t)opn2_play(myDevice, 4096, buff);
        if(got <= 0)
            break;
    }
    std::fprintf(stdout, "                                               \n\n");

    if(stop)
    {
        std::fprintf(stdout, "Interrupted! Recorded VGM is incomplete, but playable!\n");
        std::fflush(stdout);
        opn2_close(myDevice);
    }
    else
    {
        opn2_close(myDevice);
        VgmCMP::vgm_cmp_main(vgm_out, makeVgz);
        std::fprintf(stdout, "Completed!\n");
        std::fflush(stdout);
    }

    return 0;
}
