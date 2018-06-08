#
#  Project file for the Qt Creator IDE
#

TEMPLATE = lib
CONFIG  -= qt
CONFIG  += staticlib

TARGET = OPNMIDI
INSTALLINCLUDES = $$PWD/include/*
INSTALLINCLUDESTO = OPNMIDI
include($$PWD/../audio_codec_common.pri)

macx: QMAKE_CXXFLAGS_WARN_ON += -Wno-absolute-value

INCLUDEPATH += $$PWD $$PWD/include

HEADERS += \
    include/opnmidi.h \
    src/chips/gens_opn2.h \
    src/chips/gens/Ym2612_Emu.h \
    src/chips/mame/mamedef.h \
    src/chips/mame/mame_ym2612fm.h \
    src/chips/mame_opn2.h \
    src/chips/nuked_opn2.h \
    src/chips/nuked/ym3438.h \
    src/chips/gx_opn2.h \
    src/chips/gx/ym2612.h \
    src/chips/opn_chip_base.h \
    src/chips/opn_chip_base.tcc \
    src/fraction.hpp \
    src/opnbank.h \
    src/opnmidi_mus2mid.h \
    src/opnmidi_private.hpp \
    src/opnmidi_xmi2mid.h

SOURCES += \
    src/chips/gens_opn2.cpp \
    src/chips/gens/Ym2612_Emu.cpp \
    src/chips/mame/mame_ym2612fm.c \
    src/chips/mame_opn2.cpp \
    src/chips/nuked_opn2.cpp \
    src/chips/nuked/ym3438.c \
    src/chips/gx_opn2.cpp \
    src/chips/gx/ym2612.c \
    src/opnmidi.cpp \
    src/opnmidi_load.cpp \
    src/opnmidi_midiplay.cpp \
    src/opnmidi_mus2mid.c \
    src/opnmidi_opn2.cpp \
    src/opnmidi_private.cpp \
    src/opnmidi_xmi2mid.c

