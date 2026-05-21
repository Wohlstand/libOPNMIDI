
option(MIDIPLAY_PREFER_SDL  "Prefer use of SDL2 or SDL3 audio instead of WinMM/ALSA/PulseAudio" OFF)

if(UNIX AND NOT APPLE AND NOT MIDIPLAY_PREFER_SDL)
    set(USE_ALSA_AUDIO 1)
endif()

if(UNIX AND NOT APPLE AND NOT MIDIPLAY_PREFER_SDL)
    set(USE_ALSA_AUDIO 1)
endif()

if(WIN32 AND NOT MIDIPLAY_PREFER_SDL AND NOT MIDIPLAY_WAVE_ONLY)
    set(USE_WINMM_AUDIO 1)
endif()

if(MIDIPLAY_PREFER_SDL OR (NOT ADLMIDI_DOS AND NOT USE_WINMM_AUDIO AND NOT MIDIPLAY_WAVE_ONLY AND NOT USE_ALSA_AUDIO))
    libOPNMIDI_find_SDL3()
    if(TARGET OPNMIDI_SDL3)
        set(USE_SDL3_AUDIO 1)
    else()
        libOPNMIDI_find_SDL2()
        set(USE_SDL2_AUDIO 1)
    endif()
endif()

include(../common/utf8main.cmake)

set(OPNMIDI_PLAY_SRC
    opnplay.cpp
    dev_setup.cpp
    dev_setup.h
    misc.c
    misc.h
    time_counter.cpp
    time_counter.h
    audio/audio.h
    playback/playback_common.cpp
    playback/playback.h
    ${UTF8MAIN_SRCS}
)

list(APPEND OPNMIDI_PLAY_SRC
    playback/playback_wave.cpp
    playback/wave_writer.c
    playback/wave_writer.h
)

if(NOT MIDIPLAY_WAVE_ONLY)
    list(APPEND OPNMIDI_PLAY_SRC
        playback/playback_audio.cpp
    )
endif()


if(USE_SDL2_AUDIO OR USE_SDL3_AUDIO)
    list(APPEND OPNMIDI_PLAY_SRC
        audio/audio_sdl.c
    )
endif()

if(USE_ALSA_AUDIO)
    list(APPEND OPNMIDI_PLAY_SRC
        audio/audio_alsa.c
    )
endif()

if(WIN32 AND NOT USE_SDL2_AUDIO AND NOT USE_SDL3_AUDIO)
    list(APPEND OPNMIDI_PLAY_SRC
        audio/audio_winmm.c
    )
endif()

option(DEBUG_SONG_SWITCHING  "Enable song switching by left and right keys (Linux only)" OFF)
option(ENABLE_TERMINAL_HOTKEYS  "Enable use of ncurses library to handle keyboard actions" OFF)
option(TERMINAL_USE_NCURSES  "Enable use of ncurses library to handle keyboard actions" OFF)

add_executable(opnmidiplay ${OPNMIDI_PLAY_SRC})


if(USE_SDL3_AUDIO)
    target_link_libraries(opnmidiplay PRIVATE OPNMIDI_SDL3)
    target_compile_definitions(opnmidiplay PRIVATE OPNMIDI_USE_SDL3 MIDIPLAY_USE_SDL3)
elseif(USE_SDL2_AUDIO)
    target_link_libraries(opnmidiplay PRIVATE OPNMIDI_SDL2)
    target_compile_definitions(opnmidiplay PRIVATE OPNMIDI_USE_SDL2 MIDIPLAY_USE_SDL2)
elseif(USE_ALSA_AUDIO)
    target_link_libraries(opnmidiplay PRIVATE asound)
elseif(WIN32)
    target_link_libraries(opnmidiplay PRIVATE winmm)
endif()

set_nopie(opnmidiplay)

if(DEBUG_SONG_SWITCHING)
    target_compile_definitions(opnmidiplay PRIVATE DEBUG_SONG_SWITCHING)
endif()

if(MIDIPLAY_WAVE_ONLY)
    target_compile_definitions(opnmidiplay PUBLIC OUTPUT_WAVE_ONLY)
    message("Demo tool will only output WAVE file, no playing support.")
endif()

if(ENABLE_TERMINAL_HOTKEYS)
    target_compile_definitions(opnmidiplay PUBLIC ENABLE_TERMINAL_HOTKEYS)
endif()

if(TERMINAL_USE_NCURSES)
    target_compile_definitions(opnmidiplay PUBLIC TERMINAL_USE_NCURSES)
    target_link_libraries(opnmidiplay PRIVATE ncurses)
endif()

target_compile_definitions(opnmidiplay PRIVATE "-DDEFAULT_INSTALL_PREFIX=\"${CMAKE_INSTALL_PREFIX}\"")

target_link_libraries(opnmidiplay PRIVATE OPNMIDI)

if(WIN32)
    if(NOT MSVC)
        target_link_libraries(opnmidiplay PRIVATE pthread)
    endif()
    set_property(TARGET opnmidiplay PROPERTY WIN32_EXECUTABLE OFF)
    if(MSVC)
        target_compile_definitions(opnmidiplay PRIVATE -DWIN32_CONSOLE)
    endif()
else()
    if(NOT OPENBSD_LOCALBASE AND NOT HAIKU)
        target_link_libraries(opnmidiplay PRIVATE dl)
    endif()
    target_link_libraries(opnmidiplay PRIVATE pthread m)
endif()


if(libOPNMIDI_SHARED)
    add_dependencies(opnmidiplay OPNMIDI_shared)
    set_target_properties(opnmidiplay PROPERTIES INSTALL_RPATH "$ORIGIN/../lib")
else()
    if(NOT libOPNMIDI_STATIC)
        message(FATAL_ERROR "libOPNMIDI is required to be built!")
    endif()
    add_dependencies(opnmidiplay OPNMIDI_static)
endif()

if(WIN32)
    install(FILES ${libOPNMIDI_SOURCE_DIR}/fm_banks/xg.wopn DESTINATION ${CMAKE_INSTALL_BINDIR})
elseif(NOT APPLE)
    install(FILES ${libOPNMIDI_SOURCE_DIR}/fm_banks/xg.wopn DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/sounds/wopn/)
endif()

install(TARGETS opnmidiplay
        RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
        LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
        ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
        INCLUDES DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}")
