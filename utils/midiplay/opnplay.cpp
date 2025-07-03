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

    if(size != 0)
        count = _vsnprintf_s(outBuf, size, _TRUNCATE, format, ap);
    if(count == -1)
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

typedef std::deque<uint8_t> AudioBuff;
static AudioBuff g_audioBuffer;
static MutexType g_audioBuffer_lock;
static OPNMIDI_AudioFormat g_audioFormat;
static float g_gaining = 2.0f;

static void applyGain(uint8_t *buffer, size_t bufferSize)
{
    size_t i;

    switch(g_audioFormat.type)
    {
    case OPNMIDI_SampleType_S8:
    {
        int8_t *buf = reinterpret_cast<int8_t *>(buffer);
        size_t samples = bufferSize;
        for(i = 0; i < samples; ++i)
            *(buf++) *= g_gaining;
        break;
    }
    case OPNMIDI_SampleType_U8:
    {
        uint8_t *buf = buffer;
        size_t samples = bufferSize;
        for(i = 0; i < samples; ++i)
        {
            int8_t s = static_cast<int8_t>(static_cast<int32_t>(*buf) + (-0x7f - 1)) * g_gaining;
            *(buf++) = static_cast<uint8_t>(static_cast<int32_t>(s) - (-0x7f - 1));
        }
        break;
    }
    case OPNMIDI_SampleType_S16:
    {
        int16_t *buf = reinterpret_cast<int16_t *>(buffer);
        size_t samples = bufferSize / g_audioFormat.containerSize;
        for(i = 0; i < samples; ++i)
            *(buf++) *= g_gaining;
        break;
    }
    case OPNMIDI_SampleType_U16:
    {
        uint16_t *buf = reinterpret_cast<uint16_t *>(buffer);
        size_t samples = bufferSize / g_audioFormat.containerSize;
        for(i = 0; i < samples; ++i)
        {
            int16_t s = static_cast<int16_t>(static_cast<int32_t>(*buf) + (-0x7fff - 1)) * g_gaining;
            *(buf++) = static_cast<uint16_t>(static_cast<int32_t>(s) - (-0x7fff - 1));
        }
        break;
    }
    case OPNMIDI_SampleType_S32:
    {
        int32_t *buf = reinterpret_cast<int32_t *>(buffer);
        size_t samples = bufferSize / g_audioFormat.containerSize;
        for(i = 0; i < samples; ++i)
            *(buf++) *= g_gaining;
        break;
    }
    case OPNMIDI_SampleType_F32:
    {
        float *buf = reinterpret_cast<float *>(buffer);
        size_t samples = bufferSize / g_audioFormat.containerSize;
        for(i = 0; i < samples; ++i)
            *(buf++) *= g_gaining;
        break;
    }
    default:
        break;
    }
}

static void SDL_AudioCallbackX(void *, uint8_t *stream, int len)
{
    unsigned ate = static_cast<unsigned>(len); // number of bytes

    audio_lock();
    //short *target = (short *) stream;
    g_audioBuffer_lock.Lock();

    if(ate > g_audioBuffer.size())
        ate = (unsigned)g_audioBuffer.size();

    for(unsigned a = 0; a < ate; ++a)
        stream[a] = g_audioBuffer[a];

    applyGain(stream, len);

    g_audioBuffer.erase(g_audioBuffer.begin(), g_audioBuffer.begin() + ate);
    g_audioBuffer_lock.Unlock();
    audio_unlock();
}

const char* audio_format_to_str(int format, int is_msb)
{
    switch(format)
    {
    case OPNMIDI_SampleType_S8:
        return "S8";
    case OPNMIDI_SampleType_U8:
        return "U8";
    case OPNMIDI_SampleType_S16:
        return is_msb ? "S16MSB" : "S16";
    case OPNMIDI_SampleType_U16:
        return is_msb ? "U16MSB" : "U16";
    case OPNMIDI_SampleType_S32:
        return is_msb ? "S32MSB" : "S32";
    case OPNMIDI_SampleType_F32:
        return is_msb ? "F32MSB" : "F32";
    }
    return "UNK";
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

static inline void secondsToHMSM(double seconds_full, char *hmsm_buffer, size_t hmsm_buffer_size)
{
    double seconds_integral;
    double seconds_fractional = std::modf(seconds_full, &seconds_integral);
    unsigned int milliseconds = static_cast<unsigned int>(seconds_fractional * 1000.0);
    unsigned int seconds = static_cast<unsigned int>(std::fmod(seconds_full, 60.0));
    unsigned int minutes = static_cast<unsigned int>(std::fmod(seconds_full / 60, 60.0));
    unsigned int hours   = static_cast<unsigned int>(seconds_full / 3600);
    std::memset(hmsm_buffer, 0, hmsm_buffer_size);
    if(hours > 0)
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
    while(it != s.end() && std::isdigit(*it))
        ++it;
    return !s.empty() && it == s.end();
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
            " -w                Write WAV file rather than playing\n"
            " -fp               Enables full-panning stereo support\n"
            " -ea               Enable the auto-arpeggio\n"
            " --gain <value>    Set the gaining factor (default 2.0)\n"
            " --emu-mame        Use MAME YM2612 Emulator\n"
            " --emu-gens        Use GENS 2.10 Emulator\n"
            " --emu-nuked-3438  Use Nuked OPN2 YM3438 Emulator\n"
            " --emu-nuked-2612  Use Nuked OPN2 YM2612 Emulator\n"
            " --emu-nuked       Same as --emu-nuked-3438\n"
            " --emu-ymfm-opn2   Use the YMFM OPN2\n"
            // " --emu-gx          Use Genesis Plus GX Emulator\n"
            " --emu-np2         Use Neko Project II Emulator\n"
            " --emu-mame-opna   Use MAME YM2608 Emulator\n"
            " --emu-ymfm-opna   Use the YMFM OPNA\n"
            // " --emu-pmdwin      Use PMDWin Emulator\n"
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
    spec.is_msb = audio_is_big_endian();

    OPN2_MIDIPlayer *myDevice;

    /*
     * Set library options by parsing of command line arguments
     */
    bool recordWave = false;
    bool scaleModulators = false;
    bool fullRangedBrightness = false;
    int loopEnabled = 1;
    int autoArpeggioEnabled = 0;
    int chanAlloc = OPNMIDI_ChanAlloc_AUTO;
    bool fullPanEnabled = false;
    int emulator = OPNMIDI_EMU_MAME;
    int volumeModel = OPNMIDI_VolumeModel_AUTO;
    size_t soloTrack = ~static_cast<size_t>(0u);
    int songNumLoad = -1;
    int chipsCount = -1;//Auto-choose chips count by emulator (Nuked 3, others 8)
    std::vector<int> muteChannels;

    std::string bankPath;
    std::string musPath;

    int arg = 1;
    for(arg = 1; arg < argc; arg++)
    {
        if(!std::strcmp("-w", argv[arg]))
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
        else if(!std::strcmp("-frb", argv[arg]))
            fullRangedBrightness = true;
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
                return 1;
            }
            g_gaining = std::atof(argv[++arg]);
        }
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
            chipsCount = static_cast<int>(std::strtoul(argv[++arg], NULL, 10));
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

    if(!recordWave)
    {
        // Set up SDL
        if(audio_init(&spec, &obtained, SDL_AudioCallbackX) < 0)
        {
            std::fprintf(stderr, "\nERROR: Couldn't open audio: %s\n\n", audio_get_error());
            return 1;
        }

        if(spec.samples != obtained.samples || spec.freq != obtained.freq || spec.format != obtained.format)
        {
            sampleRate = obtained.freq;
            std::fprintf(stderr, " - Audio wanted (format=%s,samples=%u,rate=%u,channels=%u);\n"
                                 " - Audio obtained (format=%s,samples=%u,rate=%u,channels=%u)\n",
                         audio_format_to_str(spec.format, spec.is_msb),         spec.samples,     spec.freq,     spec.channels,
                         audio_format_to_str(obtained.format, obtained.is_msb), obtained.samples, obtained.freq, obtained.channels);
        }
    }
    else
    {
        std::memcpy(&obtained, &spec, sizeof(AudioOutputSpec));
    }

    switch(obtained.format)
    {
    case OPNMIDI_SampleType_S8:
        g_audioFormat.type = OPNMIDI_SampleType_S8;
        g_audioFormat.containerSize = sizeof(int8_t);
        g_audioFormat.sampleOffset = sizeof(int8_t) * 2;
        break;
    case OPNMIDI_SampleType_U8:
        g_audioFormat.type = OPNMIDI_SampleType_U8;
        g_audioFormat.containerSize = sizeof(uint8_t);
        g_audioFormat.sampleOffset = sizeof(uint8_t) * 2;
        break;
    case OPNMIDI_SampleType_S16:
        g_audioFormat.type = OPNMIDI_SampleType_S16;
        g_audioFormat.containerSize = sizeof(int16_t);
        g_audioFormat.sampleOffset = sizeof(int16_t) * 2;
        break;
    case OPNMIDI_SampleType_U16:
        g_audioFormat.type = OPNMIDI_SampleType_U16;
        g_audioFormat.containerSize = sizeof(uint16_t);
        g_audioFormat.sampleOffset = sizeof(uint16_t) * 2;
        break;
    case OPNMIDI_SampleType_S32:
        g_audioFormat.type = OPNMIDI_SampleType_S32;
        g_audioFormat.containerSize = sizeof(int32_t);
        g_audioFormat.sampleOffset = sizeof(int32_t) * 2;
        break;
    case OPNMIDI_SampleType_F32:
        g_audioFormat.type = OPNMIDI_SampleType_F32;
        g_audioFormat.containerSize = sizeof(float);
        g_audioFormat.sampleOffset = sizeof(float) * 2;
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
    opn2_setAutoArpeggio(myDevice, autoArpeggioEnabled);
    opn2_setChannelAllocMode(myDevice, chanAlloc);
#ifdef DEBUG_TRACE_ALL_EVENTS
    //Hook all MIDI events are ticking while generating an output buffer
    if(!recordWave)
        opn2_setRawEventHook(myDevice, debugPrintEvent, NULL);
#endif

    if(songNumLoad >= 0)
        opn2_selectSongNum(myDevice, songNumLoad);

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

    if(volumeModel != OPNMIDI_VolumeModel_AUTO)
        opn2_setVolumeRangeModel(myDevice, volumeModel);

    if(opn2_openFile(myDevice, musPath.c_str()) != 0)
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
    if(songNumLoad >= 0)
        std::fprintf(stdout, " - Attempt to load song number: %d / %d\n", songNumLoad, songsCount);
    else if(songsCount > 0)
        std::fprintf(stdout, " - File contains %d song(s)\n", songsCount);

    if(soloTrack != ~static_cast<size_t>(0u))
    {
        std::fprintf(stdout, " - Solo track: %lu\n", static_cast<unsigned long>(soloTrack));
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

    std::fprintf(stdout, " - Automatic arpeggio is turned %s\n", opn2_getAutoArpeggio(myDevice) ? "ON" : "OFF");

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

        uint8_t buff[16384];
        char posHMS[25];
        uint64_t milliseconds_prev = ~0u;
        int printsCounter = 0;
        int printsCounterPeriod = 1;

        std::fprintf(stdout, "                                               \r");

        while(!stop)
        {
            size_t got = (size_t)opn2_playFormat(myDevice, 4096,
                                                 buff,
                                                 buff + g_audioFormat.containerSize,
                                                 &g_audioFormat) * g_audioFormat.containerSize;

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
                if(printsCounter >= printsCounterPeriod)
                {
                    printsCounter = -1;
                    secondsToHMSM(time_pos, posHMS, 25);
                    std::fprintf(stdout, "                                               \r");
                    std::fprintf(stdout, "Time position: %s / %s\r", posHMS, totalHMS);
                    std::fflush(stdout);
                    milliseconds_prev = milliseconds;
                }
                printsCounter++;
            }
#endif

            g_audioBuffer_lock.Lock();
#if defined(__GNUC__) && (__GNUC__ == 15) && (__GNUC_MINOR__ == 1)
            // Workaround for GCC 15.1.0 on faulty std::deque's resize() call when C++11 is set
            // https://gcc.gnu.org/bugzilla/show_bug.cgi?id=120931
            for(size_t p = 0; p < got; ++p)
                g_audioBuffer.push_back(buff[p]);
#else
            size_t pos = g_audioBuffer.size();
            g_audioBuffer.resize(pos + got);
            for(size_t p = 0; p < got; ++p)
                g_audioBuffer[pos + p] = buff[p];
#endif
            g_audioBuffer_lock.Unlock();

            const AudioOutputSpec &spec = obtained;
            while(!stop && (g_audioBuffer.size() > static_cast<size_t>(spec.samples + (spec.freq * g_audioFormat.sampleOffset) * OurHeadRoomLength)))
            {
                audio_delay(1);
            }

#ifdef DEBUG_SEEKING_TEST
            if(delayBeforeSeek-- <= 0)
            {
                delayBeforeSeek = rand() % 50;
                double seekTo = double((rand() % int(opn2_totalTimeLength(myDevice)) - delayBeforeSeek - 1));
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
        int wav_format = obtained.format == OPNMIDI_SampleType_F32 ? WAVE_FORMAT_IEEE_FLOAT : WAVE_FORMAT_PCM;
        int wav_has_sign = obtained.format != OPNMIDI_SampleType_U8 && obtained.format != OPNMIDI_SampleType_U16;

        void *wav_ctx = ctx_wave_open(obtained.channels,
                                      static_cast<long>(sampleRate),
                                      g_audioFormat.containerSize,
                                      wav_format,
                                      wav_has_sign,
                                      (int)obtained.is_msb,
                                      wave_out.c_str()
                                      );

        if(wav_ctx)
        {
            uint8_t buff[16384];
            int complete_prev = -1;

            while(!stop)
            {
                size_t got = (size_t)opn2_playFormat(myDevice, 4096,
                                                     buff,
                                                     buff + g_audioFormat.containerSize,
                                                     &g_audioFormat) * g_audioFormat.containerSize;
                if(got <= 0)
                    break;

                applyGain(buff, got);

                ctx_wave_write(wav_ctx, buff, static_cast<long>(got));

                int complete = static_cast<int>(std::floor(100.0 * opn2_positionTell(myDevice) / total));
                if(complete_prev != complete)
                {
                    std::fprintf(stdout, "                                               \r");
                    std::fprintf(stdout, "Recording WAV... [%d%% completed]\r", complete);
                    std::fflush(stdout);
                    complete_prev = complete;
                }
            }

            ctx_wave_close(wav_ctx);
            std::fprintf(stdout, "                                               \n\n");

            if(stop)
                std::fprintf(stdout, "Interrupted! Recorded WAV is incomplete, but playable!\n");
            else
                std::fprintf(stdout, "Completed!\n");
            std::fflush(stdout);
        }
        else
        {
            std::fprintf(stdout, "Failed to open the output file: %s!\n", wave_out.c_str());
            std::fflush(stdout);
            opn2_close(myDevice);
            return 1;
        }
    }

    opn2_close(myDevice);

    return 0;
}
