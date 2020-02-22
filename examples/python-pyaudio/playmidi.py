#!/usr/bin/python3

import sys
import time
import signal
import getopt
import pyaudio
import platform

from ctypes import *

if platform.system() == 'Windows':
    opn2 = CDLL("libOPNMIDI.dll")
else:
    opn2 = CDLL("libOPNMIDI.so")

_FREQUENCY = 44100

playing = True

# Allows to quit by Ctrl+C
def catch_signal(signum, frame):
    global playing
    playing = False
    print("Caught Ctrl+C!")


class MidiPlayer:
    """A streaming IMF music player."""
    SAMPLE_SIZE = 2  # 16-bit
    CHANNELS = 2  # stereo
    BUFFER_SAMPLE_COUNT = 1024  # Max allowed: 512

    def __init__(self, freq=_FREQUENCY):
        """Initializes the PyAudio and OPL synth."""
        self._freq = freq
        # Prepare PyAudio
        self._audio = pyaudio.PyAudio()
        # Prepare buffers.
        self._data = create_string_buffer(MidiPlayer.BUFFER_SAMPLE_COUNT * MidiPlayer.SAMPLE_SIZE * MidiPlayer.CHANNELS)
        self._stream = None  # Created later.
        self._opn = opn2.opn2_init(freq)
        self._song = None

    def set_bank_file(self, bank_file):
        if opn2.opn2_openBankFile(self._opn, bank_file.encode()) < 0:
            print("%s" % c_char_p(opn2.opn2_errorInfo(self._opn)).value.decode())
            return False
        return True

    def set_loop_enabled(self, loop_en):
        opn2.opn2_setLoopEnabled(self._opn, 1 if loop_en else 0)

    def _create_stream(self, start=True):
        """Create a new PyAudio stream."""
        self._stream = self._audio.open(
            format=self._audio.get_format_from_width(MidiPlayer.SAMPLE_SIZE),
            channels=MidiPlayer.CHANNELS,
            frames_per_buffer=MidiPlayer.BUFFER_SAMPLE_COUNT,
            rate=self._freq,
            output=True,
            start=start,  # Don't start playing immediately!
            stream_callback=self._callback)

    def set_song(self, song):
        self._song = song
        if opn2.opn2_openFile(self._opn, self._song.encode()) < 0:
            print("%s" % c_char_p(opn2.opn2_errorInfo(self._opn)).value.decode())
            return False
        return True

    # noinspection PyUnusedLocal
    def _callback(self, input_data, frame_count, time_info, status):
        global playing
        got = opn2.opn2_play(self._opn, int(MidiPlayer.BUFFER_SAMPLE_COUNT * 2), cast(self._data, POINTER(c_short)));
        if got <= 0:
            playing = False
            return None, pyaudio.paComplete
        return self._data, pyaudio.paContinue

    def play(self, repeat=False):
        global playing
        """Starts playing the song at the current position."""
        self.repeat = repeat
        if self._song is None:
            playing = False
            return
        # If a stream exists and it is not active and not stopped, it needs to be closed and a new one created.
        if self._stream is not None and not self._stream.is_active() and not self._stream.is_stopped():
            self._stream.close()  # close for good measure.
            self._stream = None

        # If there's no stream at this point, create one.
        if self._stream is None:
            self._create_stream(False)
        self._stream.start_stream()

    def pause(self):
        """Stops the PyAudio stream, but does not rewind the playback position."""
        self._stream.stop_stream()

    def stop(self):
        """Stops the PyAudio stream and resets the playback position."""
        self._stream.stop_stream()

    def close(self):
        """Closes the PyAudio stream and terminates it."""
        if self._stream is not None:
            self._stream.stop_stream()
            self._stream.close()
            self._stream = None
        self._audio.terminate()
        opn2.opn2_close(self._opn)


def usage(arg):
    print('usage: %s [-b <external bank file>] [-l 0|1] <file>' % arg, file=sys.stderr)
    sys.exit(2)

if __name__ == '__main__':
    bank_file = '../../fm_banks/xg.wopn'
    enable_loop = False
    sample_rate = 44100

    opts, args = getopt.getopt(sys.argv[1:], 'b:l:')
    for o, a in opts:
        if o == '-b':
            bank_file = a
        elif o == '-l':
            enable_loop = int(a) == 1

    if not args:
        usage(sys.argv[0])

    midi_file = args[0]

    MIDIPlay = MidiPlayer(sample_rate)

    print("Using bank file: %s" % bank_file)

    if not MIDIPlay.set_bank_file(bank_file):
        exit(1)

    MIDIPlay.set_loop_enabled(enable_loop)

    if not MIDIPlay.set_song(midi_file):
        exit(1)

    MIDIPlay.play()

    signal.signal(signal.SIGINT, catch_signal)

    print("Playing MIDI file %s..." % (midi_file))
    while playing:
        time.sleep(0.1)
    MIDIPlay.stop()

    MIDIPlay.close()

