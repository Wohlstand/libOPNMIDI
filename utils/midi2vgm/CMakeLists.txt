add_executable(midi2vgm midi2vgm.cpp)
target_link_libraries(midi2vgm OPNMIDI_IF)

if(libOPNMIDI_SHARED)
    add_dependencies(midi2vgm OPNMIDI_shared)
    set_target_properties(midi2vgm PROPERTIES INSTALL_RPATH "$ORIGIN/../lib")
else()
    if(NOT libOPNMIDI_STATIC)
        message(FATAL_ERROR "libOPNMIDI is required to be built!")
    endif()
    add_dependencies(midi2vgm OPNMIDI_static)
endif()

if(NOT WIN32 AND NOT APPLE)
    install(FILES ${libOPNMIDI_SOURCE_DIR}/fm_banks/xg.wopn DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/midi2vgm/)
endif()

list(APPEND libOPNMIDI_INSTALLS midi2vgm)

