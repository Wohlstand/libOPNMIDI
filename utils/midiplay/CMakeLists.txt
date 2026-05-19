
if(UNIX AND NOT APPLE)
    set(USE_ALSA_AUDIO 1)
elseif(NOT WIN32)
    libOPNMIDI_find_SDL3()
    if(TARGET ADLMIDI_SDL3)
        set(USE_SDL3_AUDIO 1)
    else()
        libOPNMIDI_find_SDL2()
        set(USE_SDL2_AUDIO 1)
    endif()
endif()

include(../common/utf8main.cmake)

set(OPNMIDI_PLAY_SRC
    opnplay.cpp
    wave_writer.c
)

if(USE_SDL2_AUDIO OR USE_SDL3_AUDIO)
    list(APPEND OPNMIDI_PLAY_SRC
        audio_sdl.c
    )
endif()

if(USE_ALSA_AUDIO AND NOT USE_SDL2_AUDIO AND NOT USE_SDL3_AUDIO)
    list(APPEND OPNMIDI_PLAY_SRC
        audio_alsa.c
    )
endif()

if(WIN32 AND NOT USE_SDL2_AUDIO AND NOT USE_SDL3_AUDIO)
    list(APPEND OPNMIDI_PLAY_SRC
        audio_winmm.c
    )
endif()

add_executable(opnmidiplay ${OPNMIDI_PLAY_SRC})

set_nopie(opnmidiplay)

if(USE_SDL3_AUDIO)
    target_link_libraries(opnmidiplay PRIVATE OPNMIDI_SDL3)
    target_compile_definitions(opnmidiplay PRIVATE OPNMIDI_USE_SDL3)
elseif(USE_SDL2_AUDIO)
    target_link_libraries(opnmidiplay PRIVATE OPNMIDI_SDL2)
    target_compile_definitions(opnmidiplay PRIVATE OPNMIDI_USE_SDL2)
elseif(USE_ALSA_AUDIO)
    target_link_libraries(opnmidiplay PRIVATE asound)
elseif(WIN32)
    target_link_libraries(opnmidiplay PRIVATE winmm)
endif()

target_compile_definitions(opnmidiplay PRIVATE "-DDEFAULT_INSTALL_PREFIX=\"${CMAKE_INSTALL_PREFIX}\"")

target_link_libraries(opnmidiplay PRIVATE OPNMIDI)

if(WIN32)
    set_property(TARGET opnmidiplay PROPERTY WIN32_EXECUTABLE OFF)
    if(MSVC)
        target_compile_definitions(opnmidiplay PRIVATE -DWIN32_CONSOLE)
    endif()
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
