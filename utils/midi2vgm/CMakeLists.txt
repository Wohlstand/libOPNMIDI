add_executable(midi2vgm midi2vgm.cpp)
target_link_libraries(midi2vgm OPNMIDI_IF)
target_compile_definitions(midi2vgm PRIVATE "-DDEFAULT_INSTALL_PREFIX=\"${CMAKE_INSTALL_PREFIX}\"")

if(libOPNMIDI_SHARED)
    add_dependencies(midi2vgm OPNMIDI_shared)
    set_target_properties(midi2vgm PROPERTIES INSTALL_RPATH "$ORIGIN/../lib")
else()
    if(NOT libOPNMIDI_STATIC)
        message(FATAL_ERROR "libOPNMIDI is required to be built!")
    endif()
    add_dependencies(midi2vgm OPNMIDI_static)
endif()

install(TARGETS midi2vgm
        RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
        LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
        ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
        INCLUDES DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}")
