cmake_minimum_required (VERSION 2.8.11)
project (libOPNMIDI)

#===========================================================================================
# Strip garbage
if(APPLE)
    set(LINK_FLAGS_RELEASE  "${LINK_FLAGS_RELEASE} -dead_strip")
ELSEIF(NOT MSVC AND NOT MSDOS)
    set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -Os -s -fdata-sections -ffunction-sections -Wl,--gc-sections -Wl,-s")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -Os -s -fdata-sections -ffunction-sections -Wl,--gc-sections -Wl,-s")
    set(LINK_FLAGS_RELEASE  "${LINK_FLAGS_RELEASE} -Wl,--gc-sections -Wl,-s")
ENDIF()

# Global optimization flags
IF(NOT MSVC AND NOT MSDOS)
    set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -fno-omit-frame-pointer")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -fno-omit-frame-pointer")
ENDIF()

if (CMAKE_BUILD_TYPE EQUAL "RELEASE")
    add_definitions(-DNDEBUG)
ENDIF()

# Disable bogus MSVC warnings
if(MSVC)
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
ENDIF()

# -fPIC thing
IF(NOT WIN32 AND NOT DJGPP AND NOT MSDOS)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIC")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC")
ENDIF()
#===========================================================================================

option(WITH_MIDIPLAY        "Build also demo MIDI player" OFF)

option(USE_GENS_EMULATOR    "Use GENS 2.10 emulator" ON)

# WIP FEATURES
# option(WITH_CPP_EXTRAS      "Build with support for C++ extras (features are can be found in 'adlmidi.hpp' header)" OFF)

option(libOPNMIDI_STATIC   "Build static library of libOPNMIDI" ON)
option(libOPNMIDI_SHARED   "Build shared library of libOPNMIDI" OFF)

if(CMAKE_VERSION VERSION_EQUAL "3.1" OR CMAKE_VERSION VERSION_GREATER "3.1")
    set(CMAKE_CXX_STANDARD 11)
endif()

set(libOPNMIDI_INSTALLS)

include_directories(${libOPNMIDI_SOURCE_DIR}/include)
include_directories(${libOPNMIDI_SOURCE_DIR}/src/)
link_directories(${libOPNMIDI_BINARY_DIR}/)

set(libOPNMIDI_SOURCES)

list(APPEND libOPNMIDI_SOURCES
    ${libOPNMIDI_SOURCE_DIR}/src/opnmidi.cpp
    ${libOPNMIDI_SOURCE_DIR}/src/opnmidi_load.cpp
    ${libOPNMIDI_SOURCE_DIR}/src/opnmidi_midiplay.cpp
    ${libOPNMIDI_SOURCE_DIR}/src/opnmidi_opn2.cpp
    ${libOPNMIDI_SOURCE_DIR}/src/opnmidi_private.cpp
    ${libOPNMIDI_SOURCE_DIR}/src/opnmidi_mus2mid.c
    ${libOPNMIDI_SOURCE_DIR}/src/opnmidi_xmi2mid.c
)

if(USE_GENS_EMULATOR)
    add_definitions(-DUSE_LEGACY_EMULATOR)
    list(APPEND libOPNMIDI_SOURCES
        ${libOPNMIDI_SOURCE_DIR}/src/Ym2612_ChipEmu.cpp
    )
else()
    list(APPEND libOPNMIDI_SOURCES
        ${libOPNMIDI_SOURCE_DIR}/src/ym3438.c
    )
endif()

# WIP
#if(NOT WITH_CPP_EXTRAS)
#    add_definitions(-DADLMIDI_DISABLE_CPP_EXTRAS)
#endif()

if(libOPNMIDI_STATIC)
    add_library(OPNMIDI_static STATIC ${libOPNMIDI_SOURCES})
    set_target_properties(OPNMIDI_static PROPERTIES OUTPUT_NAME OPNMIDI)
    target_include_directories(OPNMIDI_static PUBLIC ${SDLMixerX_SOURCE_DIR}/include)
    list(APPEND libOPNMIDI_INSTALLS OPNMIDI_static)
endif()

if(libOPNMIDI_SHARED)
    add_library(OPNMIDI_shared SHARED ${libOPNMIDI_SOURCES})
    set_target_properties(OPNMIDI_shared PROPERTIES OUTPUT_NAME OPNMIDI)
    target_include_directories(OPNMIDI_shared PUBLIC ${SDLMixerX_SOURCE_DIR}/include)
    list(APPEND libOPNMIDI_INSTALLS OPNMIDI_shared)
endif()

if(NOT libOPNMIDI_STATIC AND NOT libOPNMIDI_SHARED)
    message(FATAL_ERROR "Both static and shared libOPNMIDI builds are disabled!
You must enable at least one of them!")
endif()

if(WITH_MIDIPLAY)
    find_library(SDL2_LIBRARY SDL2 REQUIRED)
    include_directories(${SDL2_INCLUDE_DIR})
    message("Found ${SDL2_LIBRARY}")

    add_executable(opnmidiplay
        ${libOPNMIDI_SOURCE_DIR}/utils/midiplay/opnplay.cpp
        # ${libOPNMIDI_SOURCE_DIR}/utils/midiplay/wave_writer.c # WIP
    )

    if(WIN32)
        if(MSVC)
            target_link_libraries(opnmidiplay OPNMIDI ${SDL2_LIBRARY})
        else()
            target_link_libraries(opnmidiplay OPNMIDI ${SDL2_LIBRARY} pthread)
        endif()
    else()
        target_link_libraries(opnmidiplay OPNMIDI ${SDL2_LIBRARY} pthread dl m stdc++)
    endif()

    if(libOPNMIDI_SHARED)
        add_dependencies(opnmidiplay OPNMIDI_shared)
        # ========= WIP =========
        # set_target_properties(opnmidiplay PROPERTIES COMPILE_FLAGS "-Wl,-rpath='$$ORIGIN/../lib'")
    else()
        if(NOT libOPNMIDI_STATIC)
            message(FATAL_ERROR "libOPNMIDI is required to be built!")
        endif()
        add_dependencies(opnmidiplay OPNMIDI_static)
    endif()

    list(APPEND libOPNMIDI_INSTALLS opnmidiplay)
endif()

install(TARGETS ${libOPNMIDI_INSTALLS}
        RUNTIME DESTINATION "bin"
        LIBRARY DESTINATION "lib"
        ARCHIVE DESTINATION "lib"
        INCLUDES DESTINATION "include")

install(FILES
        include/opnmidi.h
        #include/adlmidi.hpp # WIP
        DESTINATION include/)

message("==== libOPNMIDI options ====")
message("WITH_MIDIPLAY            = ${WITH_MIDIPLAY}")
message("USE_GENS_EMULATOR        = ${USE_GENS_EMULATOR}")
# message("WITH_CPP_EXTRAS          = ${WITH_CPP_EXTRAS}")
message("libOPNMIDI_STATIC        = ${libOPNMIDI_STATIC}")
message("libOPNMIDI_SHARED        = ${libOPNMIDI_SHARED}")

