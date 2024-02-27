package main

import (
    "flag"
    "log"
    "unsafe"
)

// #cgo LDFLAGS: -lOPNMIDI -Lbin
// #cgo CFLAGS:
// #include <stdio.h>
// #include <stdlib.h>
// #include <opnmidi.h>
// typedef struct wav_header {
//     // RIFF Header
//     char riff_header[4]; // Contains "RIFF"
//     int wav_size; // Size of the wav portion of the file, which follows the first 8 bytes. File size - 8
//     char wave_header[4]; // Contains "WAVE"
//     // Format Header
//     char fmt_header[4]; // Contains "fmt " (includes trailing space)
//     int fmt_chunk_size; // Should be 16 for PCM
//     short audio_format; // Should be 1 for PCM. 3 for IEEE Float
//     short num_channels;
//     int sample_rate;
//     int byte_rate; // Number of bytes per second. sample_rate * num_channels * Bytes Per Sample
//     short sample_alignment; // num_channels * Bytes Per Sample
//     short bit_depth; // Number of bits per sample
//     // Data
//     char data_header[4]; // Contains "data"
//     int data_bytes; // Number of bytes in data. Number of samples * num_channels * sample byte size
//     // uint8_t bytes[]; // Remainder of wave file is bytes
// } wav_header;
import "C"

func main() {
    inputPtr := flag.String("i", "", "input midi file")
    outputPtr := flag.String("o", "", "output wave file")
    bankPtr := flag.String("b", "", "input WOPN bank file")
    chips := flag.Int("c", 2, "chips")

    flag.Parse()

    if *bankPtr == "" {
	    log.Fatal("no input WOPN bank file")
    }

    if *inputPtr == "" {
	    log.Fatal("no input midi file")
    }

    if *outputPtr == "" {
	    log.Fatal("no output file")
    }

    // Convert into raw C-String
    cInputFile := C.CString(*inputPtr)
    cBankFile := C.CString(*bankPtr)

    // Initializing the instance of libADLMIDI
    cOpnMidiPlayer := C.opn2_init(48000)
    // Set number of parallel chips
    C.opn2_setNumChips(cOpnMidiPlayer, C.int(*chips))
    // Disable looping (it's not needed for WAV output)
    C.opn2_setLoopEnabled(cOpnMidiPlayer, 0)
    // Setting one of embedded banks
    C.opn2_openBankFile(cOpnMidiPlayer, cBankFile)
    // Open an external MIDI file to play it
    C.opn2_openFile(cOpnMidiPlayer, cInputFile)
    // Setting the channel allocation mode to the "Same instrument" mode
    C.opn2_setChannelAllocMode(cOpnMidiPlayer, 1)
    // Remove the file path buffers
    C.free(unsafe.Pointer(cInputFile))
    C.free(unsafe.Pointer(cBankFile))

    // Create the audio buffer that will be used to store taken audio chunks and write them into WAV file
    cBuffer := C.malloc(2048 * 2)

    cOutputFileName := C.CString(*outputPtr)
    cFileMode := C.CString("wb")

    cFILE := C.fopen(cOutputFileName, cFileMode)
    C.free(unsafe.Pointer(cOutputFileName))
    C.free(unsafe.Pointer(cFileMode))

    var cWavHeader C.wav_header
    cWavHeader.riff_header[0] = 'R'
    cWavHeader.riff_header[1] = 'I'
    cWavHeader.riff_header[2] = 'F'
    cWavHeader.riff_header[3] = 'F'
    cWavHeader.wav_size = 0
    cWavHeader.wave_header[0] = 'W'
    cWavHeader.wave_header[1] = 'A'
    cWavHeader.wave_header[2] = 'V'
    cWavHeader.wave_header[3] = 'E'
    cWavHeader.fmt_header[0] = 'f'
    cWavHeader.fmt_header[1] = 'm'
    cWavHeader.fmt_header[2] = 't'
    cWavHeader.fmt_header[3] = ' '
    cWavHeader.fmt_chunk_size = 16
    cWavHeader.audio_format = 1
    cWavHeader.num_channels = 2
    cWavHeader.sample_rate = 48000
    cWavHeader.byte_rate = 192000
    cWavHeader.sample_alignment = 4
    cWavHeader.bit_depth = 16
    cWavHeader.data_header[0] = 'd'
    cWavHeader.data_header[1] = 'a'
    cWavHeader.data_header[2] = 't'
    cWavHeader.data_header[3] = 'a'
    cWavHeader.data_bytes = 0

    C.fwrite(unsafe.Pointer(&cWavHeader), 44, 1, cFILE)

    finished := false
    dataBytes := 0

    for !finished {
        count := C.opn2_play(cOpnMidiPlayer, 2048, (*C.int16_t)(cBuffer))
        if C.opn2_atEnd(cOpnMidiPlayer) == 1 {
            finished = true
        }
        if count > 0 {
            dataBytes += int(count) * 4
            C.fwrite(cBuffer, 1, C.size_t(count * 2), cFILE)
        }
    }
    C.opn2_close(cOpnMidiPlayer)

    cWavHeader.wav_size = C.int(36 + dataBytes)
    cWavHeader.data_bytes = C.int(dataBytes)

    C.fseek(cFILE, 0, C.SEEK_SET)
    C.fwrite(unsafe.Pointer(&cWavHeader), 44, 1, cFILE)

    C.fclose(cFILE)
    C.free(cBuffer)
}
