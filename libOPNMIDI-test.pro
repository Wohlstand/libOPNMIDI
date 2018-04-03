TEMPLATE=app
CONFIG-=qt
CONFIG+=console

TARGET=opnplay
# DESTDIR=$$PWD/bin/

#INCLUDEPATH += /home/vitaly/_git_repos/PGE-Project/_Libs/_builds/linux/include
#LIBS += -L/home/vitaly/_git_repos/PGE-Project/_Libs/_builds/linux/lib
# LIBS += -Wl,-Bstatic -lSDL2 -Wl,-Bdynamic -lpthread -ldl
LIBS += -lSDL2 -lpthread -ldl

# DEFINES += USE_LEGACY_EMULATOR
# DEFINES += DEBUG_DUMP_RAW_STREAM

QMAKE_CFLAGS    += -std=c90 -pedantic
QMAKE_CXXFLAGS  += -std=c++98 -pedantic

INCLUDEPATH += $$PWD/include $$PWD/src

HEADERS += \
    include/opnmidi.h \
    src/chips/gens_opn2.h \
    src/chips/gens/Ym2612_Emu.h \
    src/chips/mame/mamedef.h \
    src/chips/mame/mame_ym2612fm.h \
    src/chips/mame_opn2.h \
    src/chips/nuked_opn2.h \
    src/chips/nuked/ym3438.h \
    src/chips/opn_chip_base.h \
    src/fraction.hpp \
    src/opnbank.h \
    src/opnmidi_mus2mid.h \
    src/opnmidi_private.hpp \
    src/opnmidi_xmi2mid.h

SOURCES += \
    src/chips/opn_chip_base.cpp \
    src/chips/mame_opn2.cpp \
    src/chips/gens_opn2.cpp \
    src/chips/nuked_opn2.cpp \
    src/chips/mame/mame_ym2612fm.c \
    src/chips/gens/Ym2612_Emu.cpp \
    src/chips/nuked/ym3438.c \
    src/opnmidi.cpp \
    src/opnmidi_xmi2mid.c \
    src/opnmidi_private.cpp \
    src/opnmidi_load.cpp \
    src/opnmidi_midiplay.cpp \
    src/opnmidi_mus2mid.c \
    src/opnmidi_opn2.cpp \
    utils/midiplay/opnplay.cpp
