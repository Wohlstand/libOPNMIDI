find_package(ZLIB)

add_executable(midi2vgm
    midi2vgm.cpp
    compact/vgm_cmp.cpp
    compact/chip_cmp.cpp
)

if(NOT ZLIB_FOUND)
    message(WARNING "ZLib is missing in your system, using embedded instead.")
    unset(ZLIB_LIBRARIES) # DUMMY
    include(../ZLib/zlib.cmake)
    target_sources(midi2vgm PRIVATE ${ZLIB_SRC})
    target_include_directories(midi2vgm PRIVATE ../ZLib/)
endif()

target_link_libraries(midi2vgm OPNMIDI_IF ${ZLIB_LIBRARIES})
target_compile_definitions(midi2vgm PRIVATE "-DDEFAULT_INSTALL_PREFIX=\"${CMAKE_INSTALL_PREFIX}\"")

if(WIN32)
    set_property(TARGET midi2vgm PROPERTY WIN32_EXECUTABLE OFF)
endif()

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
