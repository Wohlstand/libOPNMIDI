all: midiplay
midiplay: opnplay.cpp
	g++ $^ -O2 -Wl,-rpath='$$ORIGIN' -I.. -L../../bin -lopnmidi -lSDL2 -o ../../bin/opnmidiplay

