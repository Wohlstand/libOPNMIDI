
if(NOT WIN32)
    include(${CMAKE_CURRENT_LIST_DIR}/../../cmake/libSDL2.cmake)
    set(USE_SDL2_AUDIO 1)
elseif(WIN32)
    set(SDL2_NEEDED_LIBRARIES winmm)
endif()

set(OPNMIDI_PLAY_SRC
    opnplay.cpp
    wave_writer.c
)

if(USE_SDL2_AUDIO)
    list(APPEND OPNMIDI_PLAY_SRC
        audio_sdl.c
    )
endif()

if(WIN32 AND NOT USE_SDL2_AUDIO)
    list(APPEND OPNMIDI_PLAY_SRC
        audio_winmm.c
    )
endif()

add_executable(opnmidiplay ${OPNMIDI_PLAY_SRC})
target_link_libraries(opnmidiplay OPNMIDI_IF ${SDL2_NEEDED_LIBRARIES})
target_compile_definitions(opnmidiplay PRIVATE "-DDEFAULT_INSTALL_PREFIX=\"${CMAKE_INSTALL_PREFIX}\"")

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
