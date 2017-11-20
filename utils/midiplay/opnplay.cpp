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

#include <opnmidi.h>

#define SDL_MAIN_HANDLED
#include <SDL2/SDL.h>

#include "wave_writer.h"

class MutexType
{
    SDL_mutex *mut;
public:
    MutexType() : mut(SDL_CreateMutex()) { }
    ~MutexType()
    {
        SDL_DestroyMutex(mut);
    }
    void Lock()
    {
        SDL_mutexP(mut);
    }
    void Unlock()
    {
        SDL_mutexV(mut);
    }
};

typedef std::deque<int16_t> AudioBuff;
static AudioBuff g_audioBuffer;
static MutexType g_audioBuffer_lock;

static void SDL_AudioCallbackX(void *, Uint8 *stream, int len)
{
    SDL_LockAudio();
    short *target = reinterpret_cast<int16_t*>(stream);
    g_audioBuffer_lock.Lock();
    size_t ate = size_t(len) / 2; // number of shorts
    if(ate > g_audioBuffer.size()) ate = g_audioBuffer.size();
    for(size_t a = 0; a < ate; ++a)
        target[a] = g_audioBuffer[a];
    g_audioBuffer.erase(g_audioBuffer.begin(), g_audioBuffer.begin() + AudioBuff::difference_type(ate));
    g_audioBuffer_lock.Unlock();
    SDL_UnlockAudio();
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
    int rc = std::vsnprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);
    if(rc > 0)
    {
        std::fprintf(stdout, " - Debug: %s\n", buffer);
        std::fflush(stdout);
    }
}

#ifdef DEBUG_TRACE_ALL_EVENTS
static void debugPrintEvent(void * /*userdata*/, ADL_UInt8 type, ADL_UInt8 subtype, ADL_UInt8 channel, const ADL_UInt8 * /*data*/, size_t len)
{
    std::fprintf(stdout, " - E: 0x%02X 0x%02X %02d (%d)\r\n", type, subtype, channel, (int)len);
    std::fflush(stdout);
}
#endif

int main(int argc, char **argv)
{
    std::fprintf(stdout, "==========================================\n"
                         "         libOPNMIDI demo utility\n"
                         "==========================================\n\n");
    std::fflush(stdout);

    if(argc < 3 || std::string(argv[1]) == "--help" || std::string(argv[1]) == "-h")
    {
        std::printf(
            "Usage: opnmidi [-s] [-w] [-nl] <bankfile>.wopn <midifilename>\n"
            //" -p Enables adlib percussion instrument mode\n"
            //" -t Enables tremolo amplification mode\n"
            //" -v Enables vibrato amplification mode\n"
            " -s Enables scaling of modulator volumes\n"
            " -nl Quit without looping\n"
            " -w Write WAV file rather than playing\n"
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
    SDL_AudioSpec spec;
    SDL_AudioSpec obtained;

    spec.freq     = 44100;
    spec.format   = AUDIO_S16SYS;
    spec.channels = 2;
    spec.samples  = Uint16((double)spec.freq * AudioBufferLength);
    spec.callback = SDL_AudioCallbackX;

    OPN2_MIDIPlayer *myDevice;

    myDevice = opn2_init(44100);
    if(myDevice == NULL)
    {
        std::fprintf(stderr, "Failed to init MIDI device!\n");
        return 1;
    }

    //Set internal debug messages hook to print all libADLMIDI's internal debug messages
    opn2_setDebugMessageHook(myDevice, debugPrint, NULL);


    /*
     * Set library options by parsing of command line arguments
     */
    bool recordWave = false;
    int loopEnabled = 1;

    int arg = 1;
    for(arg = 1; arg < argc; arg++)
    {
        if(!std::strcmp("-w", argv[arg]))
            recordWave = true;//Record library output into WAV file
        else if(!std::strcmp("-nl", argv[arg]))
            loopEnabled = 0; //Enable loop
        else if(!std::strcmp("-s", argv[arg]))
            opn2_setScaleModulators(myDevice, 1);//Turn on modulators scaling by volume
        else if(!std::strcmp("--", argv[arg]))
            break;
        else
            break;
    }

    //Turn loop on/off (for WAV recording loop must be disabled!)
    opn2_setLoopEnabled(myDevice, recordWave ? 0 : loopEnabled);
    #ifdef DEBUG_TRACE_ALL_EVENTS
    //Hook all MIDI events are ticking while generating an output buffer
    if(!recordWave)
        opn2_setRawEventHook(myDevice, debugPrintEvent, NULL);
    #endif

    if(arg > argc - 2)
    {
        printError("Missing bank and/or music file paths!\n");
        return 2;
    }

    std::string bankPath = argv[arg];
    std::string musPath = argv[arg + 1];

    std::fprintf(stdout, " - %s OPN2 Emulator in use\n", opn2_emulatorName());

    if(!recordWave)
    {
        // Set up SDL
        if(SDL_OpenAudio(&spec, &obtained) < 0)
        {
            std::fprintf(stderr, "\nERROR: Couldn't open audio: %s\n\n", SDL_GetError());
            //return 1;
        }
        if(spec.samples != obtained.samples)
        {
            std::fprintf(stderr, " - Audio wanted (samples=%u,rate=%u,channels=%u);\n"
                                 " - Audio obtained (samples=%u,rate=%u,channels=%u)\n",
                         spec.samples,    spec.freq,    spec.channels,
                         obtained.samples, obtained.freq, obtained.channels);
        }
    }

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

    #ifdef USE_LEGACY_EMULATOR
    opn2_setNumChips(myDevice, 8);
    #else
    opn2_setNumCards(myDevice, 3);
    #endif

    std::fprintf(stdout, " - Number of chips %d\n", opn2_getNumChips(myDevice));

    if(opn2_openFile(myDevice, musPath.c_str()) != 0)
    {
        printError(opn2_errorInfo(myDevice));
        return 2;
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

    if(!recordWave)
    {
        std::fprintf(stdout, " - Loop is turned %s\n", loopEnabled ? "ON" : "OFF");
        if(loopStart >= 0.0 && loopEnd >= 0.0)
            std::fprintf(stdout, " - Has loop points: %10f ... %10f\n", loopStart, loopEnd);
        std::fprintf(stdout, "\n==========================================\n");
        std::fflush(stdout);

        SDL_PauseAudio(0);

        #ifdef DEBUG_SEEKING_TEST
        int delayBeforeSeek = 50;
        std::fprintf(stdout, "DEBUG: === Random position set test is active! ===\n");
        std::fflush(stdout);
        #endif

        short buff[4096];
        while(!stop)
        {
            size_t got = (size_t)opn2_play(myDevice, 4096, buff);
            if(got <= 0)
                break;

            #ifndef DEBUG_TRACE_ALL_EVENTS
            std::fprintf(stdout, "                                               \r");
            std::fprintf(stdout, "Time position: %10f / %10f\r", opn2_positionTell(myDevice), total);
            std::fflush(stdout);
            #endif

            g_audioBuffer_lock.Lock();
            size_t pos = g_audioBuffer.size();
            g_audioBuffer.resize(pos + got);
            for(size_t p = 0; p < got; ++p)
                g_audioBuffer[pos + p] = buff[p];
            g_audioBuffer_lock.Unlock();

            const SDL_AudioSpec &spec = obtained;
            while(g_audioBuffer.size() > spec.samples + (spec.freq * 2) * OurHeadRoomLength)
            {
                SDL_Delay(1);
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
        SDL_CloseAudio();
    }
    else
    {
        std::string wave_out = musPath + ".wav";
        std::fprintf(stdout, " - Recording WAV file %s...\n", wave_out.c_str());
        std::fprintf(stdout, "\n==========================================\n");
        std::fflush(stdout);

        if(wave_open(spec.freq, wave_out.c_str()) == 0)
        {
            wave_enable_stereo();
            while(!stop)
            {
                short buff[4096];
                size_t got = (size_t)opn2_play(myDevice, 4096, buff);
                if(got <= 0)
                    break;
                wave_write(buff, (long)got);

                double complete = std::floor(100.0 * opn2_positionTell(myDevice) / total);
                std::fprintf(stdout, "                                               \r");
                std::fprintf(stdout, "Recording WAV... [%d%% completed]\r", (int)complete);
                std::fflush(stdout);
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
