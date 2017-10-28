TEMPLATE=app
CONFIG-=qt
CONFIG+=console

TARGET=opnplay
DESTDIR=$$PWD/bin/

INCLUDEPATH += /home/vitaly/_git_repos/PGE-Project/_Libs/_builds/linux/include
LIBS += -L/home/vitaly/_git_repos/PGE-Project/_Libs/_builds/linux/lib
LIBS += -Wl,-Bstatic -lSDL2 -Wl,-Bdynamic -lpthread -ldl

# DEFINES += USE_LEGACY_EMULATOR
# DEFINES += DEBUG_DUMP_RAW_STREAM

HEADERS += \
    src/fraction.h \
    src/ym3438.h \
    # src/Ym2612_ChipEmu.h \
    src/opnmidi.h \
    src/opnmidi_xmi2mid.h \
    src/opnmidi_private.hpp \
    src/opnmidi_mus2mid.h \
    src/opnbank.h

SOURCES += \
    # src/Ym2612_ChipEmu.cpp \
    src/ym3438.c \
    src/midiplay/opnplay.cpp \
    src/opnmidi.cpp \
    src/opnmidi_xmi2mid.c \
    src/opnmidi_private.cpp \
    src/opnmidi_load.cpp \
    src/opnmidi_midiplay.cpp \
    src/opnmidi_mus2mid.c \
    src/opnmidi_opn2.cpp
