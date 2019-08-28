include(${CMAKE_CURRENT_LIST_DIR}/../../cmake/libSDL2.cmake)

add_executable(opnmidiplay
    opnplay.cpp
    wave_writer.c
)
target_link_libraries(opnmidiplay OPNMIDI_IF ${SDL2_NEEDED_LIBRARIES})

if(libOPNMIDI_SHARED)
    add_dependencies(opnmidiplay OPNMIDI_shared)
    set_target_properties(opnmidiplay PROPERTIES INSTALL_RPATH "$ORIGIN/../lib")
else()
    if(NOT libOPNMIDI_STATIC)
        message(FATAL_ERROR "libOPNMIDI is required to be built!")
    endif()
    add_dependencies(opnmidiplay OPNMIDI_static)
endif()

if(NOT WIN32 AND NOT APPLE)
    install(FILES ${libOPNMIDI_SOURCE_DIR}/fm_banks/xg.wopn DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/opnmidiplay/)
endif()

install(TARGETS opnmidiplay
        RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
        LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
        ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
        INCLUDES DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}")

