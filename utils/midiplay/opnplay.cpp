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
#include <stdint.h>

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

#include "audio.h"

#include "wave_writer.h"

class MutexType
{
    void *mut;
public:
    MutexType() : mut(audio_mutex_create()) { }
    ~MutexType()
    {
        audio_mutex_destroy(mut);
    }
    void Lock()
    {
        audio_mutex_lock(mut);
    }
    void Unlock()
    {
        audio_mutex_unlock(mut);
    }
};

typedef std::deque<int16_t> AudioBuff;
static AudioBuff g_audioBuffer;
static MutexType g_audioBuffer_lock;

static void SDL_AudioCallbackX(void *, uint8_t *stream, int len)
{
    audio_lock();
    short *target = reinterpret_cast<int16_t*>(stream);
    g_audioBuffer_lock.Lock();
    size_t ate = size_t(len) / 2; // number of shorts
    if(ate > g_audioBuffer.size()) ate = g_audioBuffer.size();
    for(size_t a = 0; a < ate; ++a)
        target[a] = g_audioBuffer[a];
    g_audioBuffer.erase(g_audioBuffer.begin(), g_audioBuffer.begin() + AudioBuff::difference_type(ate));
    g_audioBuffer_lock.Unlock();
    audio_unlock();
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

int main(int argc, char **argv)
{
    std::fprintf(stdout, "==========================================\n"
                         "         libOPNMIDI demo utility\n"
                         "==========================================\n\n");
    std::fflush(stdout);

    if(argc < 2 || std::string(argv[1]) == "--help" || std::string(argv[1]) == "-h")
    {
        std::printf(
            "Usage:\n"
            "   opnmidiplay [-s] [-w] [-nl] [--emu-mame|--emu-nuked|--emu-gens|--emu-gx|--emu-np2|--emu-mame-opna|--emu-pmdwin|--emu-np2-opm] \\\n"
            "               [--chips <count>] [<bankfile>.wopn] <midifilename>\n"
            "\n"
            " <bankfile>.wopn   Path to WOPN bank file\n"
            " <midifilename>    Path to music file to play\n"
            "\n"
            //" -p Enables adlib percussion instrument mode\n"
            //" -t Enables tremolo amplification mode\n"
            //" -v Enables vibrato amplification mode\n"
            " -s                Enables scaling of modulator volumes\n"
            " -frb              Enables full-ranged CC74 XG Brightness controller\n"
            " -nl               Quit without looping\n"
            " -w                Write WAV file rather than playing\n"
            " -fp               Enables full-panning stereo support\n"
            " --emu-mame        Use MAME YM2612 Emulator\n"
            " --emu-gens        Use GENS 2.10 Emulator\n"
            " --emu-nuked       Use Nuked OPN2 Emulator\n"
            " --emu-gx          Use Genesis Plus GX Emulator\n"
            " --emu-np2         Use Neko Project II OPNA Emulator\n"
            " --emu-mame-opna   Use MAME YM2608 Emulator\n"
            " --emu-pmdwin      Use PMDWin Emulator\n"
            " --emu-np2-opm     Use Neko Project II OPM Emulator\n"
            " --chips <count>   Choose a count of emulated concurrent chips\n"
            "\n"
        );
        std::fflush(stdout);

        return 0;
    }

    //const unsigned MaxSamplesAtTime = 512; // 512=dbopl limitation
    // How long is SDL buffer, in seconds?
    // The smaller the value, the more often SDL_AudioCallBack()
    // is called.
    const double AudioBufferLength = 0.08;
    // How much do WE buffer, in seconds? The smaller the value,
    // the more prone to sound chopping we are.
    const double OurHeadRoomLength = 0.1;
    // The lag between visual content and audio content equals
    // the sum of these two buffers.

    unsigned int sampleRate = 44100;

    AudioOutputSpec spec;
    AudioOutputSpec obtained;
    spec.freq     = sampleRate;
    spec.format   = OPNMIDI_SampleType_S16;
    spec.channels = 2;
    spec.samples  = uint16_t(static_cast<double>(spec.freq) * AudioBufferLength);

    OPN2_MIDIPlayer *myDevice;

    /*
     * Set library options by parsing of command line arguments
     */
    bool recordWave = false;
    bool scaleModulators = false;
    bool fullRangedBrightness = false;
    int loopEnabled = 1;
    bool fullPanEnabled = false;
    int emulator = OPNMIDI_EMU_MAME;
    size_t soloTrack = ~static_cast<size_t>(0u);
    int chipsCount = -1;//Auto-choose chips count by emulator (Nuked 3, others 8)

    std::string bankPath;
    std::string musPath;

    int arg = 1;
    for(arg = 1; arg < argc; arg++)
    {
        if(!std::strcmp("-w", argv[arg]))
            recordWave = true;//Record library output into WAV file
        else if(!std::strcmp("-frb", argv[arg]))
            fullRangedBrightness = true;
        else if(!std::strcmp("-nl", argv[arg]))
            loopEnabled = 0; //Enable loop
        else if(!std::strcmp("--emu-nuked", argv[arg]))
            emulator = OPNMIDI_EMU_NUKED;
        else if(!std::strcmp("--emu-gens", argv[arg]))
            emulator = OPNMIDI_EMU_GENS;
        else if(!std::strcmp("--emu-mame", argv[arg]))
            emulator = OPNMIDI_EMU_MAME;
        else if(!std::strcmp("--emu-gx", argv[arg]))
            emulator = OPNMIDI_EMU_GX;
        else if(!std::strcmp("--emu-np2", argv[arg]))
            emulator = OPNMIDI_EMU_NP2;
        else if(!std::strcmp("--emu-mame-opna", argv[arg]))
            emulator = OPNMIDI_EMU_MAME_2608;
        else if(!std::strcmp("--emu-pmdwin", argv[arg]))
            emulator = OPNMIDI_EMU_PMDWIN;
        else if(!std::strcmp("--emu-np2-opm", argv[arg]))
            emulator = OPNMIDI_EMU_NP2_OPM;
        else if(!std::strcmp("-fp", argv[arg]))
            fullPanEnabled = true;
        else if(!std::strcmp("-s", argv[arg]))
            scaleModulators = true;
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
            soloTrack = std::strtoul(argv[++arg], NULL, 0);
        }
        else if(!std::strcmp("--", argv[arg]))
            break;
        else
            break;
    }

    if(!recordWave)
    {
        // Set up SDL
        if(audio_init(&spec, &obtained, SDL_AudioCallbackX) < 0)
        {
            std::fprintf(stderr, "\nERROR: Couldn't open audio: %s\n\n", audio_get_error());
            return 1;
        }
        if(spec.samples != obtained.samples || spec.freq != obtained.freq)
        {
            sampleRate = obtained.freq;
            std::fprintf(stderr, " - Audio wanted (samples=%u,rate=%u,channels=%u);\n"
                                 " - Audio obtained (samples=%u,rate=%u,channels=%u)\n",
                         spec.samples,    spec.freq,    spec.channels,
                         obtained.samples, obtained.freq, obtained.channels);
        }
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
    else
    if(arg > argc - 1)
    {
        printError("Missing music file path!\n");
        return 2;
    }

    myDevice = opn2_init(sampleRate);
    if(myDevice == NULL)
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
    if(fullPanEnabled)
        opn2_setSoftPanEnabled(myDevice, 1);
    opn2_setLoopEnabled(myDevice, recordWave ? 0 : loopEnabled);
    opn2_setVolumeRangeModel(myDevice, OPNMIDI_VolumeModel_Generic);
    #ifdef DEBUG_TRACE_ALL_EVENTS
    //Hook all MIDI events are ticking while generating an output buffer
    if(!recordWave)
        opn2_setRawEventHook(myDevice, debugPrintEvent, NULL);
    #endif

    if(opn2_switchEmulator(myDevice, emulator) != 0)
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

    if(emulator == OPNMIDI_EMU_NUKED && (chipsCount < 0))
        chipsCount = 3;
    else if(chipsCount < 0)
        chipsCount = 8;
    opn2_setNumChips(myDevice, chipsCount);

    if(opn2_openFile(myDevice, musPath.c_str()) != 0)
    {
        printError(opn2_errorInfo(myDevice));
        return 2;
    }

    std::fprintf(stdout, " - Number of chips %d\n", opn2_getNumChipsObtained(myDevice));
    std::fprintf(stdout, " - Track count: %lu\n", static_cast<unsigned long>(opn2_trackCount(myDevice)));

    if(soloTrack != ~static_cast<size_t>(0u))
    {
        std::fprintf(stdout, " - Solo track: %lu\n", static_cast<unsigned long>(soloTrack));
        opn2_setTrackOptions(myDevice, soloTrack, OPNMIDI_TrackOption_Solo);
    }

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

    if(!recordWave)
    {
        std::fprintf(stdout, " - Loop is turned %s\n", loopEnabled ? "ON" : "OFF");
        if(loopStart >= 0.0 && loopEnd >= 0.0)
            std::fprintf(stdout, " - Has loop points: %s ... %s\n", loopStartHMS, loopEndHMS);
        std::fprintf(stdout, "\n==========================================\n");
        std::fflush(stdout);

        audio_start();

        #ifdef DEBUG_SEEKING_TEST
        int delayBeforeSeek = 50;
        std::fprintf(stdout, "DEBUG: === Random position set test is active! ===\n");
        std::fflush(stdout);
        #endif

        short buff[4096];
        char posHMS[25];
        uint64_t milliseconds_prev = ~0u;
        while(!stop)
        {
            size_t got = static_cast<size_t>(opn2_play(myDevice, 4096, buff));
            if(got <= 0)
                break;

#ifdef DEBUG_TRACE_ALL_CHANNELS
            enum { TerminalColumns = 80 };
            char channelText[TerminalColumns + 1];
            char channelAttr[TerminalColumns + 1];
            opn2_describeChannels(myDevice, channelText, channelAttr, sizeof(channelText));
            std::fprintf(stdout, "%*s\r", TerminalColumns, "");  // erase the line
            std::fprintf(stdout, "%s\n", channelText);
#endif

#ifndef DEBUG_TRACE_ALL_EVENTS
            double time_pos = opn2_positionTell(myDevice);
            uint64_t milliseconds = static_cast<uint64_t>(time_pos * 1000.0);
            if(milliseconds != milliseconds_prev)
            {
                secondsToHMSM(time_pos, posHMS, 25);
                std::fprintf(stdout, "                                               \r");
                std::fprintf(stdout, "Time position: %s / %s\r", posHMS, totalHMS);
                std::fflush(stdout);
                milliseconds_prev = milliseconds;
            }
#endif

            g_audioBuffer_lock.Lock();
            size_t pos = g_audioBuffer.size();
            g_audioBuffer.resize(pos + got);
            for(size_t p = 0; p < got; ++p)
                g_audioBuffer[pos + p] = buff[p];
            g_audioBuffer_lock.Unlock();

            const AudioOutputSpec &cur_spec = obtained;
            while(!stop && (g_audioBuffer.size() > static_cast<size_t>(cur_spec.samples + (cur_spec.freq * 2) * OurHeadRoomLength)))
            {
                audio_delay(1);
            }

#ifdef DEBUG_SEEKING_TEST
            if(delayBeforeSeek-- <= 0)
            {
                delayBeforeSeek = rand() % 50;
                double seekTo = double((rand() % int(opn2_totalTimeLength(myDevice)) - delayBeforeSeek - 1 ));
                opn2_positionSeek(myDevice, seekTo);
            }
#endif
        }
        std::fprintf(stdout, "                                               \n\n");
        audio_stop();
        audio_close();
    }
    else
    {
        std::string wave_out = musPath + ".wav";
        std::fprintf(stdout, " - Recording WAV file %s...\n", wave_out.c_str());
        std::fprintf(stdout, "\n==========================================\n");
        std::fflush(stdout);

        if(wave_open(static_cast<int>(sampleRate), wave_out.c_str()) == 0)
        {
            wave_enable_stereo();
            short buff[4096];
            int complete_prev = -1;
            while(!stop)
            {
                size_t got = static_cast<size_t>(opn2_play(myDevice, 4096, buff));
                if(got <= 0)
                    break;

                wave_write(buff, static_cast<long>(got));

                int complete = static_cast<int>(std::floor(100.0 * opn2_positionTell(myDevice) / total));
                if(complete_prev != complete)
                {
                    std::fprintf(stdout, "                                               \r");
                    std::fprintf(stdout, "Recording WAV... [%d%% completed]\r", complete);
                    std::fflush(stdout);
                    complete_prev = complete;
                }
            }
            wave_close();
            std::fprintf(stdout, "                                               \n\n");

            if(stop)
                std::fprintf(stdout, "Interrupted! Recorded WAV is incomplete, but playable!\n");
            else
                std::fprintf(stdout, "Completed!\n");
            std::fflush(stdout);
        }
        else
        {
            opn2_close(myDevice);
            return 1;
        }
    }

    opn2_close(myDevice);

    return 0;
}
