/* Copyright (C) 2011, 2012 Sergey V. Mikayev
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License as published by
 *  the Free Software Foundation, either version 2.1 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "stdafx.h"

#include "gm_opn_bank.h"

namespace OPL3Emu
{

#define DRIVER_MODE

static MidiSynth &midiSynth = MidiSynth::getInstance();

static const unsigned int s_audioChannels = 2;

static class MidiStream
{
private:
    static const unsigned int maxPos = 1024;
    unsigned int startpos;
    unsigned int endpos;
    DWORD stream[maxPos][2];

public:
    MidiStream()
    {
        startpos = 0;
        endpos = 0;
    }

    DWORD PutMessage(DWORD msg, DWORD timestamp)
    {
        unsigned int newEndpos = endpos;

        newEndpos++;
        if(newEndpos == maxPos)  // check for buffer rolloff
            newEndpos = 0;
        if(startpos == newEndpos)  // check for buffer full
            return -1;
        stream[endpos][0] = msg;    // ok to put data and update endpos
        stream[endpos][1] = timestamp;
        endpos = newEndpos;
        return 0;
    }

    DWORD GetMidiMessage()
    {
        if(startpos == endpos)  // check for buffer empty
            return -1;
        DWORD msg = stream[startpos][0];
        startpos++;
        if(startpos == maxPos)  // check for buffer rolloff
            startpos = 0;
        return msg;
    }

    DWORD PeekMessageTime()
    {
        if(startpos == endpos)  // check for buffer empty
            return (DWORD) -1;
        return stream[startpos][1];
    }

    DWORD PeekMessageTimeAt(unsigned int pos)
    {
        if(startpos == endpos)  // check for buffer empty
            return -1;
        unsigned int peekPos = (startpos + pos) % maxPos;
        return stream[peekPos][1];
    }

    void Clean()
    {
        startpos = 0;
        endpos = 0;
        memset(stream, 0, sizeof(stream));
    }

} midiStream;

static class SynthEventWin32
{
private:
    HANDLE hEvent;

public:
    int Init()
    {
        hEvent = CreateEvent(NULL, false, true, NULL);
        if(hEvent == NULL)
        {
            MessageBoxW(NULL, L"Can't create sync object", L"libOPNMIDI", MB_OK | MB_ICONEXCLAMATION);
            return 1;
        }
        return 0;
    }

    void Close()
    {
        CloseHandle(hEvent);
    }

    void Wait()
    {
        WaitForSingleObject(hEvent, INFINITE);
    }

    void Release()
    {
        SetEvent(hEvent);
    }
} synthEvent;

static class WaveOutWin32
{
private:
    HWAVEOUT    hWaveOut;
    WAVEHDR     *WaveHdr;
    HANDLE      hEvent;
    DWORD       chunks;
    DWORD       prevPlayPos;
    DWORD       getPosWraps;
    bool        stopProcessing;
    WORD        formatType;
    DWORD       sizeSample;

public:
    WaveOutWin32() :
        hWaveOut(NULL),
        WaveHdr(NULL),
        hEvent(NULL),
        chunks(0),
        prevPlayPos(0),
        getPosWraps(0),
        stopProcessing(false),
        formatType(0),
        sizeSample(0)
    {}

    int Init(OPN2_UInt8 *buffer, size_t bufferSize,
             WORD formatTag,
             unsigned int chunkSize, bool useRingBuffer,
             unsigned int sampleRate, UINT outDevice)
    {
        DWORD callbackType = CALLBACK_NULL;
        DWORD_PTR callback = (DWORD_PTR)NULL;
        size_t numFrames;

        if(hWaveOut)
            Close();

        formatType = formatTag;
        sizeSample = formatType == WAVE_FORMAT_IEEE_FLOAT ? sizeof(float) : sizeof(Bit16s);
        numFrames = bufferSize / (sizeSample * s_audioChannels);

        if(!useRingBuffer)
        {
            hEvent = CreateEvent(NULL, false, true, NULL);
            callback = (DWORD_PTR)hEvent;
            callbackType = CALLBACK_EVENT;
        }

        PCMWAVEFORMAT wFormat =
        {
            formatType, s_audioChannels,
            sampleRate,
            (DWORD)(sampleRate * sizeSample * s_audioChannels),
            (WORD)(s_audioChannels * sizeSample),
            (WORD)(8 * sizeSample)
        };

        // Open waveout device
        int wResult = waveOutOpen(&hWaveOut, outDevice, (LPWAVEFORMATEX)&wFormat,
                                  callback, (DWORD_PTR)&midiSynth, callbackType);

        if(wResult != MMSYSERR_NOERROR)
        {
            if(formatType != WAVE_FORMAT_IEEE_FLOAT)
                MessageBoxW(NULL, L"Failed to open waveform output device", L"libOPNMIDI", MB_OK | MB_ICONEXCLAMATION);

            Close();
            return 2;
        }

        // Prepare headers
        chunks = useRingBuffer ? 1 : numFrames / chunkSize;
        WaveHdr = new WAVEHDR[chunks];

        LPSTR chunkStart = (LPSTR)buffer;
        DWORD chunkBytes = s_audioChannels * sizeSample * chunkSize;

        for(UINT i = 0; i < chunks; i++)
        {
            if(useRingBuffer)
            {
                WaveHdr[i].dwBufferLength = bufferSize;
                WaveHdr[i].lpData = chunkStart;
                WaveHdr[i].dwFlags = WHDR_BEGINLOOP | WHDR_ENDLOOP;
                WaveHdr[i].dwLoops = -1L;
            }
            else
            {
                WaveHdr[i].dwBufferLength = chunkBytes;
                WaveHdr[i].lpData = chunkStart;
                WaveHdr[i].dwFlags = 0L;
                WaveHdr[i].dwLoops = 0L;
                chunkStart += chunkBytes;
            }

            wResult = waveOutPrepareHeader(hWaveOut, &WaveHdr[i], sizeof(WAVEHDR));
            if(wResult != MMSYSERR_NOERROR)
            {
                MessageBoxW(NULL, L"Failed to Prepare Header", L"libOPNMIDI", MB_OK | MB_ICONEXCLAMATION);
                return 3;
            }
        }

        stopProcessing = false;
        return 0;
    }

    int Close()
    {
        int wResult;

        stopProcessing = true;
        if(hEvent != NULL)
            SetEvent(hEvent);

        if(hWaveOut != NULL)
        {
            wResult = waveOutReset(hWaveOut);
            if(wResult != MMSYSERR_NOERROR)
            {
                MessageBoxW(NULL, L"Failed to Reset WaveOut", L"libOPNMIDI", MB_OK | MB_ICONEXCLAMATION);
                return 8;
            }
        }

        for(UINT i = 0; i < chunks; i++)
        {
            wResult = waveOutUnprepareHeader(hWaveOut, &WaveHdr[i], sizeof(WAVEHDR));
            if(wResult != MMSYSERR_NOERROR)
            {
                MessageBoxW(NULL, L"Failed to Unprepare Wave Header", L"libOPNMIDI", MB_OK | MB_ICONEXCLAMATION);
                return 8;
            }
        }

        if(WaveHdr != NULL)
            delete[] WaveHdr;
        WaveHdr = NULL;

        if(hWaveOut != NULL)
        {
            wResult = waveOutClose(hWaveOut);
            if(wResult != MMSYSERR_NOERROR)
            {
                MessageBoxW(NULL, L"Failed to Close WaveOut", L"libOPNMIDI", MB_OK | MB_ICONEXCLAMATION);
                return 8;
            }
        }

        hWaveOut = NULL;

        if(hEvent != NULL)
        {
            CloseHandle(hEvent);
            hEvent = NULL;
        }

        return 0;
    }

    int Start()
    {
        HANDLE renderThread;
        getPosWraps = 0;
        prevPlayPos = 0;

        for(UINT i = 0; i < chunks; i++)
        {
            if(waveOutWrite(hWaveOut, &WaveHdr[i], sizeof(WAVEHDR)) != MMSYSERR_NOERROR)
            {
                MessageBoxW(NULL, L"Failed to write block to device", L"libOPNMIDI", MB_OK | MB_ICONEXCLAMATION);
                return 4;
            }
        }

        renderThread = (HANDLE)_beginthread(RenderingThread, 8192 * sizeSample, this);
        SetThreadPriority(renderThread, THREAD_PRIORITY_HIGHEST);
        return 0;
    }

    int Pause()
    {
        if(waveOutPause(hWaveOut) != MMSYSERR_NOERROR)
        {
            MessageBoxW(NULL, L"Failed to Pause wave playback", L"libOPNMIDI", MB_OK | MB_ICONEXCLAMATION);
            return 9;
        }
        return 0;
    }

    int Resume()
    {
        if(waveOutRestart(hWaveOut) != MMSYSERR_NOERROR)
        {
            MessageBoxW(NULL, L"Failed to Resume wave playback", L"libOPNMIDI", MB_OK | MB_ICONEXCLAMATION);
            return 9;
        }
        return 0;
    }

    UINT64 GetPos()
    {
        MMTIME mmTime;
        mmTime.wType = TIME_SAMPLES;

        if(waveOutGetPosition(hWaveOut, &mmTime, sizeof(MMTIME)) != MMSYSERR_NOERROR)
        {
            MessageBoxW(NULL, L"Failed to get current playback position", L"libOPNMIDI", MB_OK | MB_ICONEXCLAMATION);
            return 10;
        }

        if(mmTime.wType != TIME_SAMPLES)
        {
            MessageBoxW(NULL, L"Failed to get # of samples played", L"libOPNMIDI", MB_OK | MB_ICONEXCLAMATION);
            return 10;
        }

        // Deal with waveOutGetPosition() wraparound. For 16-bit stereo output, it equals 2^27,
        // presumably caused by the internal 32-bit counter of bits played.
        // The output of that nasty waveOutGetPosition() isn't monotonically increasing
        // even during 2^27 samples playback, so we have to ensure the difference is big enough...
        int delta = mmTime.u.sample - prevPlayPos;
        if(delta < -(1 << 26))
        {
            std::cout << "OPN2: GetPos() wrap: " << delta << "\n";
            std::cout.flush();
            ++getPosWraps;
        }

        prevPlayPos = mmTime.u.sample;
        return mmTime.u.sample + getPosWraps * (1 << 27);
    }

    static void RenderingThread(void *);
} s_waveOut;

void WaveOutWin32::RenderingThread(void *)
{
    if(s_waveOut.chunks == 1)
    {
        // Rendering using single looped ring buffer
        while(!s_waveOut.stopProcessing)
            midiSynth.RenderAvailableSpace();
    }
    else
    {
        while(!s_waveOut.stopProcessing)
        {
            bool allBuffersRendered = true;

            for(UINT i = 0; i < s_waveOut.chunks; i++)
            {
                if(s_waveOut.WaveHdr[i].dwFlags & WHDR_DONE)
                {
                    allBuffersRendered = false;
                    midiSynth.Render((Bit8u *)s_waveOut.WaveHdr[i].lpData,
                                     s_waveOut.WaveHdr[i].dwBufferLength);

                    if(waveOutWrite(s_waveOut.hWaveOut, &s_waveOut.WaveHdr[i], sizeof(WAVEHDR)) != MMSYSERR_NOERROR)
                        MessageBoxW(NULL, L"Failed to write block to device", L"libOPNMIDI", MB_OK | MB_ICONEXCLAMATION);

                    midiSynth.CheckForSignals();
                }
            }

            if(allBuffersRendered)
                WaitForSingleObject(s_waveOut.hEvent, INFINITE);
        }
    }
}


MidiSynth::MidiSynth() :
    buffer(NULL),
    bufferSizeB(0),
    synth(NULL)
{
    m_setupInit = false;
    useRingBuffer = false;
    volumeFactorL = 1.0f;
    volumeFactorR = 1.0f;
    gain = 1.0f;

    setupDefault(&m_setup);
    loadSetup();
    ::openSignalListener();
}

MidiSynth::~MidiSynth()
{
    if(synth)
        opn2_close(synth);

    synth = NULL;
    ::closeSignalListener();
}

MidiSynth &MidiSynth::getInstance()
{
    static MidiSynth *instance = new MidiSynth;
    return *instance;
}

// Renders all the available space in the single looped ring buffer
void MidiSynth::RenderAvailableSpace()
{
    DWORD playPos = s_waveOut.GetPos() % bufferSize;
    DWORD framesToRender;

    if(playPos < framesRendered)
    {
        // Buffer wrap, render 'till the end of the buffer
        framesToRender = bufferSize - framesRendered;
    }
    else
    {
        framesToRender = playPos - framesRendered;
        if(framesToRender < chunkSize)
        {
            Sleep(1 + (chunkSize - framesToRender) * 1000 / sampleRate);
            return;
        }
    }

    midiSynth.Render(buffer + (synthAudioFormat.containerSize * framesRendered), framesToRender);
}

// Renders totalFrames frames starting from bufpos
// The number of frames rendered is added to the global counter framesRendered
void MidiSynth::Render(Bit8u *bufpos_p, DWORD bufSize)
{
    DWORD totalFrames = bufSize / (synthAudioFormat.containerSize * s_audioChannels);

    while(totalFrames > 0)
    {
        DWORD timeStamp;
        // Incoming MIDI messages timestamped with the current audio playback position + midiLatency
        while((timeStamp = midiStream.PeekMessageTime()) == framesRendered)
        {
            DWORD msg = midiStream.GetMidiMessage();

            synthEvent.Wait();

            Bit8u event = msg & 0xFF;
            Bit8u channel = msg & 0x0F;
            Bit8u p1 = (msg >> 8) & 0x7f;
            Bit8u p2 = (msg >> 16) & 0x7f;

            event &= 0xF0;

            if(event == 0xF0)
            {
                switch (channel)
                {
                case 0xF:
                    opn2_reset(synth);
                    break;
                }
            }

            switch(event & 0xF0)
            {
            case 0x80:
                opn2_rt_noteOff(synth, channel, p1);
                break;
            case 0x90:
                opn2_rt_noteOn(synth, channel, p1, p2);
                break;
            case 0xA0:
                opn2_rt_noteAfterTouch(synth, channel, p1, p2);
                break;
            case 0xB0:
                opn2_rt_controllerChange(synth, channel, p1, p2);
                break;
            case 0xC0:
                opn2_rt_patchChange(synth, channel, p1);
                break;
            case 0xD0:
                opn2_rt_channelAfterTouch(synth, channel, p1);
                break;
            case 0xE0:
                opn2_rt_pitchBendML(synth, channel, p2, p1);
                break;
            }

            synthEvent.Release();
        }

        // Find out how many frames to render. The value of timeStamp == -1 indicates the MIDI buffer is empty
        DWORD framesToRender = timeStamp - framesRendered;
        if(framesToRender > totalFrames)
        {
            // MIDI message is too far - render the rest of frames
            framesToRender = totalFrames;
        }

        synthEvent.Wait();
        opn2_generateFormat(synth, framesToRender * s_audioChannels,
                            bufpos_p, bufpos_p + synthAudioFormat.containerSize,
                            &synthAudioFormat);
        // Apply the volume
        float g_l = volumeFactorL * gain;
        float g_r = volumeFactorR * gain;

        if(synthAudioFormat.type == OPNMIDI_SampleType_F32)
        {
            float *bufpos = (float *)bufpos_p;
            for(size_t i = 0; i < framesToRender * s_audioChannels; i += s_audioChannels)
            {
                bufpos[i + 0] *= g_l;
                bufpos[i + 1] *= g_r;
            }
        }
        else
        {
            Bit16s *bufpos = (Bit16s *)bufpos_p;
            for(size_t i = 0; i < framesToRender * s_audioChannels; i += s_audioChannels)
            {
                bufpos[i + 0] *= g_l;
                bufpos[i + 1] *= g_r;
            }
        }

        synthEvent.Release();

        framesRendered += framesToRender;
        // each frame consists of two samples for both the Left and Right channels
        bufpos_p += s_audioChannels * framesToRender * synthAudioFormat.containerSize;
        totalFrames -= framesToRender;
    }

    // Wrap framesRendered counter
    if(framesRendered >= bufferSize)
        framesRendered -= bufferSize;
}

void MidiSynth::CheckForSignals()
{
    int cmd = ::hasReloadSetupSignal();

    if(cmd == 0)
        return;

    switch(cmd)
    {
    case DRV_SIGNAL_RELOAD_SETUP: // Reload settings on the fly
        this->loadSetup();
        LoadSynthSetup();
        break;

    case DRV_SIGNAL_RESET_SYNTH:
        opn2_reset(synth);
        break;

    case DRV_SIGNAL_UPDATE_GAIN:
        this->loadGain();
        break;

    default:
        break;
    }

    if(cmd > 0)
        ::resetSignal();
}

unsigned int MidiSynth::MillisToFrames(unsigned int millis)
{
    return UINT(sampleRate * millis / 1000.f);
}

void MidiSynth::LoadSettings()
{
    sampleRate = 49716;
    bufferSize = MillisToFrames(100);
    chunkSize = MillisToFrames(10);
    midiLatency = MillisToFrames(0);

    if(!useRingBuffer)
    {
        // Number of chunks should be ceil(bufferSize / chunkSize)
        DWORD chunks = (bufferSize + chunkSize - 1) / chunkSize;
        // Refine bufferSize as chunkSize * number of chunks, no less then the specified value
        bufferSize = chunks * chunkSize;
    }
}

int MidiSynth::Init()
{
    LoadSettings();

    if(!buffer)
    {
        bufferSizeB = s_audioChannels * bufferSize * sizeof(float);
        buffer = (OPN2_UInt8 *)malloc(bufferSizeB); // each frame consists of two samples for both the Left and Right channels
    }

    // Init synth
    if(synthEvent.Init())
        return 1;

    synth = opn2_init(49716);
    if(!synth)
    {
        MessageBoxW(NULL, L"Can't open Synth", L"libOPNMIDI", MB_OK | MB_ICONEXCLAMATION);
        return 1;
    }

    synthAudioFormat.type = OPNMIDI_SampleType_F32;
    synthAudioFormat.sampleOffset = s_audioChannels * sizeof(float);
    synthAudioFormat.containerSize = sizeof(float);

    m_setupInit = false;
    LoadSynthSetup();

    UINT wResult = s_waveOut.Init(buffer, bufferSizeB,
                                  WAVE_FORMAT_IEEE_FLOAT,
                                  chunkSize, useRingBuffer, sampleRate, m_setup.outputDevice);

    if(wResult)
    {
        synthAudioFormat.type = OPNMIDI_SampleType_S16;
        synthAudioFormat.sampleOffset = s_audioChannels * sizeof(Bit16s);
        synthAudioFormat.containerSize = sizeof(Bit16s);

        bufferSizeB = s_audioChannels * bufferSize * sizeof(Bit16s);
        buffer = (OPN2_UInt8 *)realloc(buffer, bufferSizeB); // Shrink the buffer
        wResult = s_waveOut.Init(buffer, bufferSizeB,
                                         WAVE_FORMAT_PCM,
                                         chunkSize, useRingBuffer, sampleRate, m_setup.outputDevice);
    }

    if(wResult)
        return wResult;

    // Start playing stream
    opn2_generateFormat(synth, bufferSize * s_audioChannels,
                        buffer,
                        buffer + synthAudioFormat.containerSize,
                        &synthAudioFormat);
    framesRendered = 0;

    wResult = s_waveOut.Start();
    return wResult;
}

int MidiSynth::Reset()
{
#ifdef DRIVER_MODE
    return 0;
#endif

    UINT wResult = s_waveOut.Pause();
    if(wResult) return wResult;

    synthEvent.Wait();

    if(synth)
        opn2_close(synth);

    synth = opn2_init(49716);
    if(!synth)
    {
        MessageBoxW(NULL, L"Can't open Synth", L"libOPNMIDI", MB_OK | MB_ICONEXCLAMATION);
        return 1;
    }

    m_setupInit = false;
    LoadSynthSetup();

    synthEvent.Release();

    wResult = s_waveOut.Resume();
    return wResult;
}

void MidiSynth::ResetSynth()
{
    synthEvent.Wait();
    opn2_reset(synth);
    midiStream.Clean();
    synthEvent.Release();
}

void MidiSynth::PanicSynth()
{
    synthEvent.Wait();
    opn2_panic(synth);
    synthEvent.Release();
}

void MidiSynth::PushMIDI(DWORD msg)
{
    midiStream.PutMessage(msg, (s_waveOut.GetPos() + midiLatency) % bufferSize);
}

void MidiSynth::PlaySysex(Bit8u *bufpos, DWORD len)
{
    synthEvent.Wait();
    opn2_rt_systemExclusive(synth, bufpos, len);
    synthEvent.Release();
}

void MidiSynth::SetVolume(DWORD vol)
{
    volumeFactorR = (float)0xFFFF / HIWORD(vol);
    volumeFactorL = (float)0xFFFF / LOWORD(vol);
}

DWORD MidiSynth::GetVolume()
{
    return MAKELONG((DWORD)(0xFFFF * volumeFactorL), (DWORD)(0xFFFF * volumeFactorR));
}

void MidiSynth::loadSetup()
{
    ::loadSetup(&m_setup);
    gain = (float)m_setup.gain100 / 100.f;
}

void MidiSynth::loadGain()
{
    ::getGain(&m_setup);
    gain = (float)m_setup.gain100 / 100.f;
}

void MidiSynth::LoadSynthSetup()
{
    int inEmulatorId = m_setup.emulatorId;

    if(inEmulatorId >= OPNMIDI_VGM_DUMPER)
        inEmulatorId++; // Skip the VGM dumper

    if(!m_setupInit || m_setupCurrent.emulatorId != inEmulatorId)
    {
        opn2_switchEmulator(synth, inEmulatorId);
        m_setupCurrent.emulatorId = inEmulatorId;
    }

    if(!m_setupInit || m_setupCurrent.numChips != m_setup.numChips)
    {
        opn2_setNumChips(synth, m_setup.numChips);
        m_setupCurrent.numChips = m_setup.numChips;
    }

    if(!m_setupInit || m_setupCurrent.flagSoftPanning != m_setup.flagSoftPanning)
    {
        opn2_setSoftPanEnabled(synth, m_setup.flagSoftPanning);
        m_setupCurrent.flagSoftPanning = m_setup.flagSoftPanning;
    }


    if(!m_setupInit || m_setupCurrent.flagScaleModulators != m_setup.flagScaleModulators)
    {
        opn2_setScaleModulators(synth, m_setup.flagScaleModulators);
        m_setupCurrent.flagScaleModulators = m_setup.flagScaleModulators;
    }

    if(!m_setupInit || m_setupCurrent.flagFullBrightness != m_setup.flagFullBrightness)
    {
        opn2_setFullRangeBrightness(synth, m_setup.flagFullBrightness);
        m_setupCurrent.flagFullBrightness = m_setup.flagFullBrightness;
    }

    if(!m_setupInit || m_setupCurrent.volumeModel != m_setup.volumeModel)
    {
        opn2_setVolumeRangeModel(synth, m_setup.volumeModel);
        m_setupCurrent.volumeModel = m_setup.volumeModel;
    }

    if(!m_setupInit || m_setupCurrent.chanAlloc != m_setup.chanAlloc)
    {
        opn2_setChannelAllocMode(synth, m_setup.chanAlloc);
        m_setupCurrent.chanAlloc = m_setup.chanAlloc;
    }

    if(!m_setupInit || m_setupCurrent.numChips != m_setup.numChips)
    {
        opn2_setNumChips(synth, m_setup.numChips);
        m_setupCurrent.numChips = m_setup.numChips;
    }

    if(!m_setupInit ||
       m_setupCurrent.useExternalBank != m_setup.useExternalBank ||
       wcscmp(m_setupCurrent.bankPath, m_setup.bankPath) != 0
    )
    {
        if(m_setup.useExternalBank)
        {
            char pathUtf8[MAX_PATH * 4];
            ZeroMemory(pathUtf8, MAX_PATH * 4);
            int len = WideCharToMultiByte(CP_UTF8, 0, m_setup.bankPath, wcslen(m_setup.bankPath), pathUtf8, MAX_PATH * 4, 0, 0);
            pathUtf8[len] = '\0';
            opn2_openBankFile(synth, pathUtf8);
        }
        else
            opn2_openBankData(synth, g_xg_wopn_bank, sizeof(g_xg_wopn_bank));

        m_setupCurrent.useExternalBank = m_setup.useExternalBank;
        wcscpy(m_setupCurrent.bankPath, m_setup.bankPath);
    }

    m_setupInit = true;
}

void MidiSynth::Close()
{
    s_waveOut.Pause();
    s_waveOut.Close();
    synthEvent.Wait();
    //synth->close();

    // Cleanup memory
    if(synth)
        opn2_close(synth);
    synth = NULL;

    if(buffer)
        free(buffer);
    buffer = NULL;

    synthEvent.Close();
}

}
