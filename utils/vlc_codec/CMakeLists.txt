set(CMAKE_C_STANDARD 99)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/../../cmake")
find_package(LIBVLC REQUIRED)
message("Found ${LIBVLCCORE_LIBRARY} of version ${LIBVLC_VERSION} with includes ${LIBVLC_INCLUDE_DIR}")

set(opnmidi_vlc_src)
list(APPEND opnmidi_vlc_src libopnmidi.c)

add_library(opnmidi_vlc_plugin MODULE ${opnmidi_vlc_src})
set_target_properties(opnmidi_vlc_plugin PROPERTIES OUTPUT_NAME opnmidi_plugin)
target_compile_options(opnmidi_vlc_plugin PUBLIC
            "-DVLC_MODULE_COPYRIGHT=\"Copyright \(c\) Vitaly Novichkov\""
            "-DVLC_MODULE_LICENSE=\"GPLv3\""
            "-DMODULE_STRING=\"opnmidi\""
            ${VLCPLUGIN_DEFINITIONS})

if(USE_GX_EMULATOR)
    target_compile_options(opnmidi_vlc_plugin PRIVATE -DUSE_GX_EMULATOR)
endif()

target_include_directories(opnmidi_vlc_plugin PRIVATE
        ${LIBVLC_INCLUDE_DIR}/vlc/plugins
        ${LIBVLC_INCLUDE_DIR}/vlc)

set(OPNMIDIVLC_LIBS OPNMIDI_IF_STATIC ${LIBVLCCORE_LIBRARY} m)

if(CMAKE_SYSTEM_NAME STREQUAL "Windows" AND CMAKE_COMPILER_IS_GNUCXX)
    set_property(TARGET opnmidi_vlc_plugin APPEND_STRING PROPERTY LINK_FLAGS " -static-libgcc -static-libstdc++")
    set_property(TARGET opnmidi_vlc_plugin APPEND_STRING PROPERTY LINK_FLAGS " -Wl,-Bstatic,--whole-archive -lpthread -Wl,-Bdynamic,--no-whole-archive")
elseif(NOT WIN32)
    list(APPEND OPNMIDIVLC_LIBS stdc++)
endif()

target_link_libraries(opnmidi_vlc_plugin ${OPNMIDIVLC_LIBS})
# target_link_libraries(opnmidi_vlc_plugin INTERFACE "-export-symbol-regex ^vlc_entry")

if(NOT VLC_PLUGIN_NOINSTALL)
    install(TARGETS opnmidi_vlc_plugin DESTINATION "${VLCPLUGIN_CODEC_INSTALL_PATH}")
else()
    install(TARGETS opnmidi_vlc_plugin DESTINATION "${CMAKE_INSTALL_LIBDIR}/vlc-codec/")
endif()

add_dependencies(opnmidi_vlc_plugin OPNMIDI_static)
