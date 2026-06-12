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

#include <string>
#include <cstdio>
#include <cctype>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <cstdarg>
#include <signal.h>

#include "utf8main.h" // IWYU pragma: keep

#include <opnmidi.h>

#include "misc.h"
#include "time_counter.h"
#include "dev_setup.h"
#include "playback/playback.h"


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

static void sighandler(int dum)
{
    switch(dum)
    {
    case SIGINT:
    case SIGTERM:
#ifndef _WIN32
    case SIGHUP:
#endif
        stop = 1;
        break;
    default:
        break;
    }
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


int main(int argc, char **argv)
{
    s_fprintf(stdout,
                "==========================================\n"
                "         libOPNMIDI demo utility\n"
                "==========================================\n\n");
    flushout(stdout);

    bool doQuit = false;
    int parseRet = s_devSetup.parseArgs(argc, argv, &doQuit);

    if(doQuit)
        return parseRet;

    OPN2_MIDIPlayer *myDevice;

    myDevice = opn2_init(s_devSetup.sampleRate);
    if(myDevice == NULL)
    {
        std::fprintf(stderr, "Failed to init MIDI device!\n");
        return 1;
    }

    //Set internal debug messages hook to print all libADLMIDI's internal debug messages
    opn2_setDebugMessageHook(myDevice, debugPrint, NULL);

#if !defined(OUTPUT_WAVE_ONLY)
    g_audioFormat.type = OPNMIDI_SampleType_S16;
    g_audioFormat.containerSize = sizeof(int16_t);
    g_audioFormat.sampleOffset = sizeof(int16_t) * 2;
#endif

    //Turn loop on/off (for WAV recording loop must be disabled!)
    if(s_devSetup.scaleModulators)
        opn2_setScaleModulators(myDevice, 1);//Turn on modulators scaling by volume
    if(s_devSetup.fullRangedBrightness)
        opn2_setFullRangeBrightness(myDevice, 1);//Turn on a full-ranged XG CC74 Brightness
    if(s_devSetup.fullPanEnabled)
        opn2_setSoftPanEnabled(myDevice, 1);

#ifndef OUTPUT_WAVE_ONLY
    //Turn loop on/off (for WAV recording loop must be disabled!)
    opn2_setLoopEnabled(myDevice, s_devSetup.recordWave ? 0 : s_devSetup.loopEnabled);
#endif

    opn2_setModeEMIDI(myDevice, s_devSetup.modeEMIDI);

    opn2_setVolumeRangeModel(myDevice, OPNMIDI_VolumeModel_Generic);
    opn2_setAutoArpeggio(myDevice, s_devSetup.autoArpeggioEnabled);
    opn2_setChannelAllocMode(myDevice, s_devSetup.chanAlloc);

#ifdef DEBUG_TRACE_ALL_EVENTS
    //Hook all MIDI events are ticking while generating an output buffer
    if(!s_devSetup.recordWave)
        opn2_setRawEventHook(myDevice, debugPrintEvent, NULL);
#endif

    if(s_devSetup.songNumLoad >= 0)
        opn2_selectSongNum(myDevice, s_devSetup.songNumLoad);

    if(opn2_switchEmulator(myDevice, s_devSetup.emulator) != 0)
    {
        std::fprintf(stdout, "FAILED!\n");
        std::fflush(stdout);
        printError(opn2_errorInfo(myDevice));
        return 2;
    }

    std::fprintf(stdout, " - Library version %s\n", opn2_linkedLibraryVersion());
    std::fprintf(stdout, " - %s Emulator in use\n", opn2_chipEmulatorName(myDevice));

    std::fprintf(stdout, " - Use bank [%s]...", s_devSetup.bankPath.c_str());
    std::fflush(stdout);
    if(opn2_openBankFile(myDevice, s_devSetup.bankPath.c_str()) != 0)
    {
        std::fprintf(stdout, "FAILED!\n");
        std::fflush(stdout);
        printError(opn2_errorInfo(myDevice));
        return 2;
    }
    std::fprintf(stdout, "OK!\n");

    if(s_devSetup.emulator == OPNMIDI_EMU_NUKED && (s_devSetup.chipsCount < 0))
        s_devSetup.chipsCount = 3;
    else if(s_devSetup.chipsCount < 0)
        s_devSetup.chipsCount = 8;
    opn2_setNumChips(myDevice, s_devSetup.chipsCount);

    if(s_devSetup.volumeModel != OPNMIDI_VolumeModel_AUTO)
        opn2_setVolumeRangeModel(myDevice, s_devSetup.volumeModel);

    if(opn2_openFile(myDevice, s_devSetup.musPath.c_str()) != 0)
    {
        printError(opn2_errorInfo(myDevice));
        return 2;
    }

    std::fprintf(stdout, " - Number of chips %d\n", opn2_getNumChipsObtained(myDevice));
    std::fprintf(stdout, " - Track count: %lu\n", static_cast<unsigned long>(opn2_trackCount(myDevice)));
    std::fprintf(stdout, " - Volume model: %s\n", volume_model_to_str(opn2_getVolumeRangeModel(myDevice)));
    std::fprintf(stdout, " - Channel allocation mode: %s\n", chanalloc_to_str(opn2_getChannelAllocMode(myDevice)));

    std::fprintf(stdout, " - Gain level: %g\n", g_gaining);

    int songsCount = opn2_getSongsCount(myDevice);
    if(s_devSetup.songNumLoad >= 0)
        std::fprintf(stdout, " - Attempt to load song number: %d / %d\n", s_devSetup.songNumLoad, songsCount);
    else if(songsCount > 0)
        std::fprintf(stdout, " - File contains %d song(s)\n", songsCount);

    if(s_devSetup.soloTrack != ~static_cast<size_t>(0u))
    {
        std::fprintf(stdout, " - Solo track: %lu\n", static_cast<unsigned long>(s_devSetup.soloTrack));
        opn2_setTrackOptions(myDevice, s_devSetup.soloTrack, OPNMIDI_TrackOption_Solo);
    }

    if(!s_devSetup.muteChannels.empty())
    {
        std::printf(" - Mute MIDI channels:");

        for(size_t i = 0; i < s_devSetup.muteChannels.size(); ++i)
        {
            opn2_setChannelEnabled(myDevice, s_devSetup.muteChannels[i], 0);
            std::printf(" %d", s_devSetup.muteChannels[i]);
        }

        std::printf("\n");
        std::fflush(stdout);
    }

    std::fprintf(stdout, " - Automatic arpeggio is turned %s\n", opn2_getAutoArpeggio(myDevice) ? "ON" : "OFF");

    std::fprintf(stdout, " - File [%s] opened!\n", s_devSetup.musPath.c_str());
    std::fflush(stdout);

    signal(SIGINT, sighandler);
    signal(SIGTERM, sighandler);
#ifndef _WIN32
    signal(SIGHUP, sighandler);
#endif

    s_timeCounter.setTotal(opn2_totalTimeLength(myDevice));

#ifndef OUTPUT_WAVE_ONLY
    s_timeCounter.setLoop(opn2_loopStartTime(myDevice), opn2_loopEndTime(myDevice));

    if(!s_devSetup.recordWave)
    {
        s_fprintf(stdout, " - Loop is turned %s\n", s_devSetup.loopEnabled ? "ON" : "OFF");
        if(s_timeCounter.hasLoop)
            s_fprintf(stdout, " - Has loop points: %s ... %s\n", s_timeCounter.loopStartHMS, s_timeCounter.loopEndHMS);
        s_fprintf(stdout, "\n==========================================\n");

        s_fprintf(stdout, "Playing... Hit Ctrl+C to quit!\n");
        flushout(stdout);

        int ret = runAudioLoop(myDevice, s_devSetup.spec);
        if(ret != 0)
        {
            opn2_close(myDevice);
            return ret;
        }

        s_timeCounter.clearLine();
    }
    else
#endif //OUTPUT_WAVE_ONLY
    {
        int ret = runWaveOutLoopLoop(myDevice, s_devSetup.musPath, s_devSetup.spec, s_devSetup.sampleRate);
        if(ret != 0)
        {
            opn2_close(myDevice);
            return ret;
        }
    }

    opn2_close(myDevice);

    return 0;
}
