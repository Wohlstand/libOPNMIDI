all: midiplay
midiplay: opnplay.cpp
	g++ $^ -Wl,-rpath='$$ORIGIN' -I.. -L../../bin -lopnmidi -lSDL2 -o ../../bin/opnmidiplay

