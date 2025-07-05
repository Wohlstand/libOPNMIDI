cmake_minimum_required (VERSION 3.2...3.10)
project(libOPNMIDI VERSION 1.6.0 LANGUAGES C CXX)

include(GNUInstallDirs)
include(CheckCXXCompilerFlag)

# Prefer C90 standard
set(CMAKE_C_STANDARD 90)
# Prefer C++98 standard
set(CMAKE_CXX_STANDARD 98)

if(MSVC)
    check_cxx_compiler_flag("/std:c++14" COMPILER_SUPPORTS_CXX14)
    if(COMPILER_SUPPORTS_CXX14)
        set(FLAG_CPP14 "/std:c++14")
    endif()
    if(MSVC_VERSION LESS 1910)
        unset(COMPILER_SUPPORTS_CXX14) # MSVC older than 2017 fails to build YMFM
    endif()
else()
    check_cxx_compiler_flag("-std=c++14" COMPILER_SUPPORTS_CXX14)
    if(COMPILER_SUPPORTS_CXX14)
        set(FLAG_CPP14 "-std=c++14")
    endif()
endif()

if(COMPILER_SUPPORTS_CXX14)
    message("== Your C++ compiler supports C++14, YMFM emulator will be ENABLED")
else()
    message("== Your C++ compiler does NOT supports C++14, YMFM emulator will be DISABLED")
endif()

if(NOT CMAKE_RUNTIME_OUTPUT_DIRECTORY)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
endif()
if(NOT CMAKE_LIBRARY_OUTPUT_DIRECTORY)
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
endif()
if(NOT CMAKE_ARCHIVE_OUTPUT_DIRECTORY)
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
endif()

#===========================================================================================
# Strip garbage
if(APPLE)
    set(LINK_FLAGS_RELEASE  "${LINK_FLAGS_RELEASE} -dead_strip")
elseif(NOT MSVC AND NOT MSDOS AND NOT OPENBSD_LOCALBASE)
    string(REGEX REPLACE "-O3" ""
            CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE}")
    string(REGEX REPLACE "-O3" ""
            CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE}")
    set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -O2 -fdata-sections -ffunction-sections")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O2 -fdata-sections -ffunction-sections")
    if(ANDROID)
        set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -funwind-tables")
        set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -funwind-tables")
    endif()
    if(NOT "${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
        set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -s -Wl,--gc-sections -Wl,-s")
        set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -s -Wl,--gc-sections -Wl,-s")
        set(LINK_FLAGS_RELEASE  "${LINK_FLAGS_RELEASE} -Wl,--gc-sections -Wl,-s")
    endif()
endif()

# Global optimization flags
if(NOT MSVC AND NOT MSDOS)
# Global optimization flags
    set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -fno-omit-frame-pointer")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -fno-omit-frame-pointer")
# Turn on all warnings
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra")
# Deny undefined symbols
    if(NOT APPLE)
        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,--no-undefined" )
        set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -Wl,--no-undefined" )
        set(CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} -Wl,--no-undefined" )
    endif()
# Supress the std::vector::insert() GCC change warning
    if(VITA)
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DVITA -DVITA=1 -fcompare-debug-second")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DVITA -DVITA=1 -fcompare-debug-second")
    endif()
    if(NINTENDO_3DS OR NINTENDO_WII OR NINTENDO_WIIU OR NINTENDO_SWITCH)
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fcompare-debug-second")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fcompare-debug-second")
    endif()
endif()

if(CMAKE_COMPILER_IS_GNUCC OR CMAKE_COMPILER_IS_GNUCXX OR CMAKE_C_COMPILER_ID STREQUAL "Clang")
    if(NINTENDO_3DS OR NINTENDO_WII OR NINTENDO_WIIU OR NINTENDO_SWITCH)
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=gnu90")
    else()
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c89")
    endif()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++98")
endif()

if(DEFINED CMAKE_BUILD_TYPE)
    string(TOLOWER "${CMAKE_BUILD_TYPE}" CMAKE_BUILD_TYPE_LOWER)
endif()
if(CMAKE_BUILD_TYPE_LOWER EQUAL "release")
    add_definitions(-DNDEBUG)
ENDIF()

# Disable bogus MSVC warnings
if(MSVC)
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
ENDIF()

if(OPENBSD_LOCALBASE)
    message("OpenBSD Detected!")
    include_directories(/usr/local/include)
endif()

if(WIN32)
    add_definitions(-DUNICODE -D_UNICODE)
endif()

function(set_legacy_standard destTarget)
    if(CMAKE_COMPILER_IS_GNUCC OR CMAKE_COMPILER_IS_GNUCXX)
        # Turn on warnings and legacy C/C++ standards to support more compilers
        target_compile_options(${destTarget} PRIVATE
            $<$<COMPILE_LANGUAGE:C>:-Wall -pedantic -std=gnu90>
            $<$<COMPILE_LANGUAGE:CXX>:-Wall -pedantic -std=gnu++98>
        )
    endif()
endfunction()

function(set_visibility_hidden destTarget)
    if(CMAKE_C_COMPILER_ID MATCHES "^(GNU|Clang)$" AND NOT DJGPP)
        if(${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION} VERSION_GREATER 3.2)
            target_compile_options(${destTarget} PRIVATE
              $<$<COMPILE_LANGUAGE:CXX>:-fvisibility-inlines-hidden> -fvisibility=hidden)
        else()
            target_compile_options(${destTarget} PRIVATE -fvisibility=hidden)
        endif()
        target_compile_definitions(${destTarget} PRIVATE -DLIBOPNMIDI_VISIBILITY)
    endif()
endfunction()
#===========================================================================================

if(VITA OR NINTENDO_3DS OR NINTENDO_WII OR NINTENDO_WIIU)
    set(DEFAULT_HEAVY_EMULATORS OFF)
else()
    set(DEFAULT_HEAVY_EMULATORS ON)
endif()

if(NOT EMSCRIPTEN
   AND NOT VITA
   AND NOT NINTENDO_3DS
   AND NOT NINTENDO_WII
   AND NOT NINTENDO_WIIU
   AND NOT NINTENDO_SWITCH
   AND NOT MSDOS)
    set(libOPNMIDI_STATIC_ENABLED_BY_DEFAULT ON)
    set(libOPNMIDI_SHARED_ENABLED_BY_DEFAULT OFF)

    # When defined, respect CMake's BUILD_SHARED_LIBS setting
    if (BUILD_SHARED_LIBS)
        set(libOPNMIDI_SHARED_ENABLED_BY_DEFAULT ON)
        set(libOPNMIDI_STATIC_ENABLED_BY_DEFAULT OFF)
    endif()

    option(libOPNMIDI_STATIC   "Build static library of libOPNMIDI" ${libOPNMIDI_STATIC_ENABLED_BY_DEFAULT})
    option(libOPNMIDI_SHARED   "Build shared library of libOPNMIDI" ${libOPNMIDI_SHARED_ENABLED_BY_DEFAULT})
else()
    set(libOPNMIDI_STATIC ON)
    set(libOPNMIDI_SHARED OFF)
endif()

option(WITH_MIDI_SEQUENCER  "Build with embedded MIDI sequencer. Disable this if you want use library in real-time MIDI drivers or plugins.)" ON)
option(WITH_HQ_RESAMPLER    "Build with support for high quality resampling" OFF)
option(WITH_MUS_SUPPORT     "Build with support for DMX MUS files)" ON)
option(WITH_XMI_SUPPORT     "Build with support for AIL XMI files)" ON)
option(USE_MAME_EMULATOR    "Use MAME YM2612 emulator (for most of hardware)" ON)
option(USE_GENS_EMULATOR    "Use GENS 2.10 emulator (fastest, very outdated, inaccurate)" ON)
option(USE_NUKED_EMULATOR   "Use Nuked OPN2 emulator (most accurate, heavy)" ${DEFAULT_HEAVY_EMULATORS})
# option(USE_GX_EMULATOR      "Use Genesis Plus GX emulator (experimental)" OFF)
option(USE_NP2_EMULATOR     "Use Neko Project II emulator" ON)
option(USE_MAME_2608_EMULATOR "Use MAME YM2608 emulator" ON)
# option(USE_PMDWIN_EMULATOR  "Use PMDWin emulator (experimental)" OFF)
option(USE_VGM_FILE_DUMPER  "Use VGM File Dumper (required to build the MIDI2VGM tool)" ON)
if(COMPILER_SUPPORTS_CXX14)
    option(USE_YMFM_EMULATOR    "Use YMFM emulator (requires C++14 support)" ON)
endif()
# WIP FEATURES
# option(WITH_CPP_EXTRAS      "Build with support for C++ extras (features are can be found in 'adlmidi.hpp' header)" OFF)

option(WITH_MIDIPLAY        "Build also demo MIDI player" OFF)
option(WITH_MIDI2VGM        "Build also MIDI to VGM converter tool" OFF)
option(WITH_WOPN2HPP        "Build also the WOPN to C++ header source converter tool" OFF)
option(WITH_VLC_PLUGIN      "Build also a plugin for VLC Media Player" OFF)
option(VLC_PLUGIN_NOINSTALL "Don't install VLC plugin into VLC directory" OFF)
option(WITH_DAC_UTIL        "Build also OPN2 DAC testing utility" OFF)

option(WITH_EXTRA_BANKS     "Install extra bank files" OFF)

if(WIN32)
    option(WITH_WINMMDRV    "Build a WinMM MIDI driver" OFF)
endif()

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
    ${libOPNMIDI_SOURCE_DIR}/src/wopn/wopn_file.c
)

# Make headers being properly visible at IDEs
file(GLOB libOPNMIDI_HEADERS
    ${libOPNMIDI_SOURCE_DIR}/include/*.h
    ${libOPNMIDI_SOURCE_DIR}/src/*.h
    ${libOPNMIDI_SOURCE_DIR}/src/*.hpp
    ${libOPNMIDI_SOURCE_DIR}/src/wopn/*.h
    ${libOPNMIDI_SOURCE_DIR}/src/structures/*.hpp
    ${libOPNMIDI_SOURCE_DIR}/src/chips/*.h
    ${libOPNMIDI_SOURCE_DIR}/src/chips/gens/*.hpp
    ${libOPNMIDI_SOURCE_DIR}/src/chips/gx/*.h
    ${libOPNMIDI_SOURCE_DIR}/src/chips/mame/*.h
    ${libOPNMIDI_SOURCE_DIR}/src/chips/mamefm/*.h
    ${libOPNMIDI_SOURCE_DIR}/src/chips/np2/*.h
    ${libOPNMIDI_SOURCE_DIR}/src/chips/nuked/*.h
    ${libOPNMIDI_SOURCE_DIR}/src/chips/pmdwin/*.h
    ${libOPNMIDI_SOURCE_DIR}/src/chips/ymfm/*.h
    ${libOPNMIDI_SOURCE_DIR}/src/chips/ymfm/*.hpp
)
list(APPEND libOPNMIDI_SOURCES ${libOPNMIDI_HEADERS})

if(WITH_MIDI_SEQUENCER)
    list(APPEND libOPNMIDI_SOURCES
        ${libOPNMIDI_SOURCE_DIR}/src/opnmidi_sequencer.cpp
    )
    add_definitions(-DENABLE_END_SILENCE_SKIPPING)
endif()

if(NOT WITH_MUS_SUPPORT OR NOT WITH_MIDI_SEQUENCER)
    add_definitions(-DBWMIDI_DISABLE_MUS_SUPPORT)
endif()

if(NOT WITH_XMI_SUPPORT OR NOT WITH_MIDI_SEQUENCER)
    add_definitions(-DBWMIDI_DISABLE_XMI_SUPPORT)
endif()

if(USE_GENS_EMULATOR)
    list(APPEND libOPNMIDI_SOURCES
        ${libOPNMIDI_SOURCE_DIR}/src/chips/gens_opn2.cpp
        ${libOPNMIDI_SOURCE_DIR}/src/chips/gens/Ym2612.cpp
    )
    set(HAS_EMULATOR TRUE)
else()
    add_definitions(-DOPNMIDI_DISABLE_GENS_EMULATOR)
endif()

if(USE_MAME_EMULATOR)
    list(APPEND libOPNMIDI_SOURCES
        ${libOPNMIDI_SOURCE_DIR}/src/chips/mame_opn2.cpp
        ${libOPNMIDI_SOURCE_DIR}/src/chips/mame/mame_ym2612fm.c
    )
    set(HAS_EMULATOR TRUE)
else()
    add_definitions(-DOPNMIDI_DISABLE_MAME_EMULATOR)
endif()

if(USE_NUKED_EMULATOR)
    list(APPEND libOPNMIDI_SOURCES
        ${libOPNMIDI_SOURCE_DIR}/src/chips/nuked_opn2.cpp
        ${libOPNMIDI_SOURCE_DIR}/src/chips/nuked/ym3438.c
    )
    set(HAS_EMULATOR TRUE)
else()
    add_definitions(-DOPNMIDI_DISABLE_NUKED_EMULATOR)
endif()

#if(USE_GX_EMULATOR)
#    list(APPEND libOPNMIDI_SOURCES
#        ${libOPNMIDI_SOURCE_DIR}/src/chips/gx_opn2.cpp
#        ${libOPNMIDI_SOURCE_DIR}/src/chips/gx/gx_ym2612.c
#    )
#    set(HAS_EMULATOR TRUE)
#else()
#    add_definitions(-DOPNMIDI_DISABLE_GX_EMULATOR)
#endif()

if(USE_NP2_EMULATOR)
    list(APPEND libOPNMIDI_SOURCES
        ${libOPNMIDI_SOURCE_DIR}/src/chips/np2_opna.cpp
        ${libOPNMIDI_SOURCE_DIR}/src/chips/np2/fmgen_opna.cpp
        ${libOPNMIDI_SOURCE_DIR}/src/chips/np2/fmgen_file.cpp
        ${libOPNMIDI_SOURCE_DIR}/src/chips/np2/fmgen_fmgen.cpp
        ${libOPNMIDI_SOURCE_DIR}/src/chips/np2/fmgen_fmtimer.cpp
        ${libOPNMIDI_SOURCE_DIR}/src/chips/np2/fmgen_opna.cpp
        ${libOPNMIDI_SOURCE_DIR}/src/chips/np2/fmgen_psg.cpp)
    set(HAS_EMULATOR TRUE)
else()
    add_definitions(-DOPNMIDI_DISABLE_NP2_EMULATOR)
endif()

if(USE_MAME_2608_EMULATOR)
    list(APPEND libOPNMIDI_SOURCES
        ${libOPNMIDI_SOURCE_DIR}/src/chips/mame_opna.cpp
        ${libOPNMIDI_SOURCE_DIR}/src/chips/mamefm/fm.cpp
        ${libOPNMIDI_SOURCE_DIR}/src/chips/mamefm/ymdeltat.cpp
        ${libOPNMIDI_SOURCE_DIR}/src/chips/mamefm/emu2149.c
        ${libOPNMIDI_SOURCE_DIR}/src/chips/mamefm/resampler.cpp)
    set(HAS_EMULATOR TRUE)
else()
    add_definitions(-DOPNMIDI_DISABLE_MAME_2608_EMULATOR)
endif()

#if(USE_PMDWIN_EMULATOR)
#    list(APPEND libOPNMIDI_SOURCES
#        ${libOPNMIDI_SOURCE_DIR}/src/chips/pmdwin_opna.cpp
#        ${libOPNMIDI_SOURCE_DIR}/src/chips/pmdwin/opna.c
#        ${libOPNMIDI_SOURCE_DIR}/src/chips/pmdwin/psg.c
#        ${libOPNMIDI_SOURCE_DIR}/src/chips/pmdwin/rhythmdata.c)
#    set(HAS_EMULATOR TRUE)
#else()
#    add_definitions(-DOPNMIDI_DISABLE_PMDWIN_EMULATOR)
#endif()

if(USE_VGM_FILE_DUMPER)
    list(APPEND libOPNMIDI_SOURCES
        ${libOPNMIDI_SOURCE_DIR}/src/chips/vgm_file_dumper.cpp)
    set(HAS_EMULATOR TRUE)
    add_definitions(-DOPNMIDI_MIDI2VGM)
endif()

if(USE_YMFM_EMULATOR)
    set(YMFM_SOURCES
        "${libOPNMIDI_SOURCE_DIR}/src/chips/ymfm_opn2.cpp"
        "${libOPNMIDI_SOURCE_DIR}/src/chips/ymfm_opna.cpp"
        "${libOPNMIDI_SOURCE_DIR}/src/chips/ymfm/ymfm_opn.cpp"
        "${libOPNMIDI_SOURCE_DIR}/src/chips/ymfm/ymfm_misc.cpp"
        "${libOPNMIDI_SOURCE_DIR}/src/chips/ymfm/ymfm_pcm.cpp"
        "${libOPNMIDI_SOURCE_DIR}/src/chips/ymfm/ymfm_adpcm.cpp"
        "${libOPNMIDI_SOURCE_DIR}/src/chips/ymfm/ymfm_ssg.cpp"
    )
    if(DEFINED FLAG_CPP14)
        set_source_files_properties(${YMFM_SOURCES} COMPILE_FLAGS ${FLAG_CPP14})
    endif()
    list(APPEND libOPNMIDI_SOURCES ${YMFM_SOURCES})
    set(HAS_EMULATOR TRUE)
else()
    add_definitions(-DOPNMIDI_DISABLE_YMFM_EMULATOR)
endif()

if(NOT HAS_EMULATOR)
    message(FATAL_ERROR "No emulators enabled! You should enable at least one emulator!")
endif()

if(NOT WITH_MIDI_SEQUENCER)
    add_definitions(-DOPNMIDI_DISABLE_MIDI_SEQUENCER)
endif()

if(NOT WIN32
   AND NOT VITA
   AND NOT NINTENDO_3DS
   AND NOT NINTENDO_WII
   AND NOT NINTENDO_WIIU
   AND NOT EMSCRIPTEN
   AND NOT MSDOS)
    option(ENABLE_FPIC "Build static libraries as position independent code" ON)
else()
    set(ENABLE_FPIC OFF)
endif()

# === Static library ====
if(libOPNMIDI_STATIC OR WITH_VLC_PLUGIN)
    add_library(OPNMIDI_static STATIC ${libOPNMIDI_SOURCES})
    if(WIN32 AND libOPNMIDI_SHARED)
        set_target_properties(OPNMIDI_static PROPERTIES OUTPUT_NAME OPNMIDI-static)
    else()
        set_target_properties(OPNMIDI_static PROPERTIES OUTPUT_NAME OPNMIDI)
    endif()
    target_include_directories(OPNMIDI_static PUBLIC $<BUILD_INTERFACE:${libOPNMIDI_SOURCE_DIR}/include> $<INSTALL_INTERFACE:include>)
    set_legacy_standard(OPNMIDI_static)
    set_visibility_hidden(OPNMIDI_static)

    # -fPIC thing
    if(ENABLE_FPIC)
        set_target_properties(OPNMIDI_static PROPERTIES COMPILE_FLAGS "-fPIC")
    endif()

    if(NOT libOPNMIDI_STATIC)
        set_target_properties(OPNMIDI_static PROPERTIES EXCLUDE_FROM_ALL TRUE)
    endif()
endif()

# === Shared library ====
if(libOPNMIDI_SHARED)
    add_library(OPNMIDI_shared SHARED ${libOPNMIDI_SOURCES})
    set_target_properties(OPNMIDI_shared PROPERTIES OUTPUT_NAME OPNMIDI)
    set_target_properties(OPNMIDI_shared PROPERTIES
        VERSION ${libOPNMIDI_VERSION}
        SOVERSION ${libOPNMIDI_VERSION_MAJOR}
    )
    target_include_directories(OPNMIDI_shared PUBLIC $<BUILD_INTERFACE:${libOPNMIDI_SOURCE_DIR}/include> $<INSTALL_INTERFACE:include>)
    set_legacy_standard(OPNMIDI_shared)
    set_visibility_hidden(OPNMIDI_shared)

    if(WIN32)
        target_compile_definitions(OPNMIDI_shared PRIVATE "-DOPNMIDI_BUILD_DLL")
        if(CMAKE_COMPILER_IS_GNUCXX)
            set_property(TARGET OPNMIDI_shared APPEND_STRING PROPERTY LINK_FLAGS " -static-libgcc -static-libstdc++")
            set_property(TARGET OPNMIDI_shared APPEND_STRING PROPERTY LINK_FLAGS " -Wl,-Bstatic,--whole-archive -lpthread -Wl,-Bdynamic,--no-whole-archive")
        endif()
    endif()

    if(OPENBSD_LOCALBASE)
        set_property(TARGET OPNMIDI_shared APPEND_STRING PROPERTY LINK_FLAGS " -lc")
    endif()

    set_target_properties(OPNMIDI_shared PROPERTIES SOVERSION "1")
endif()

if(NOT libOPNMIDI_STATIC AND NOT libOPNMIDI_SHARED)
    message(FATAL_ERROR "Both static and shared libOPNMIDI builds are disabled!
You must enable at least one of them!")
endif()

add_library(OPNMIDI_IF INTERFACE)

if(libOPNMIDI_STATIC OR WITH_VLC_PLUGIN)
    add_library(OPNMIDI_IF_STATIC INTERFACE)
    target_link_libraries(OPNMIDI_IF_STATIC INTERFACE OPNMIDI_static)
endif()

if(libOPNMIDI_SHARED)
  target_link_libraries(OPNMIDI_IF INTERFACE OPNMIDI_shared)
else()
  target_link_libraries(OPNMIDI_IF INTERFACE OPNMIDI_static)
endif()

if(WITH_MIDIPLAY)
    add_subdirectory(utils/midiplay)
endif()

if(WITH_MIDI2VGM)
    if(NOT USE_VGM_FILE_DUMPER)
        message(FATAL_ERROR "To build MIDI to VGM converter, you must enable -DUSE_VGM_FILE_DUMPER=ON flag!")
    endif()
    add_subdirectory(utils/midi2vgm)
endif()

if(WITH_WOPN2HPP)
    add_subdirectory(utils/wopn2hpp)
endif()

if(WITH_VLC_PLUGIN)
    add_subdirectory(utils/vlc_codec)
endif()

if(WITH_DAC_UTIL)
    add_subdirectory(utils/dac_test)
endif()

if(WIN32 AND WITH_WINMMDRV)
    add_subdirectory(utils/winmm_drv)
endif()

if(WITH_HQ_RESAMPLER)
    find_library(ZITA_RESAMPLER_LIBRARY "zita-resampler" REQUIRED)
    add_definitions(-DOPNMIDI_ENABLE_HQ_RESAMPLER)
    if(libOPNMIDI_SHARED)
        target_link_libraries(OPNMIDI_shared PUBLIC "${ZITA_RESAMPLER_LIBRARY}")
    endif()
    if(libOPNMIDI_STATIC OR WITH_VLC_PLUGIN)
        target_link_libraries(OPNMIDI_static PUBLIC "${ZITA_RESAMPLER_LIBRARY}")
    endif()
endif()

if(libOPNMIDI_STATIC)
    install(TARGETS OPNMIDI_static
            EXPORT libOPNMIDIStaticTargets
            RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
            LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
            ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}")

    install(EXPORT libOPNMIDIStaticTargets
            FILE libOPNMIDI-static-targets.cmake
            NAMESPACE libOPNMIDI::
            DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/libOPNMIDI")
endif()

if(libOPNMIDI_SHARED)
    install(TARGETS OPNMIDI_shared
            EXPORT libOPNMIDISharedTargets
            RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
            LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
            ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}")

    install(EXPORT libOPNMIDISharedTargets
            FILE libOPNMIDI-shared-targets.cmake
            NAMESPACE libOPNMIDI::
            DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/libOPNMIDI")
endif()

install(FILES
        include/opnmidi.h
        #include/opnmidi.hpp # WIP
        DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}")

include(CMakePackageConfigHelpers)
configure_package_config_file(libOPNMIDIConfig.cmake.in "${CMAKE_CURRENT_BINARY_DIR}/libOPNMIDIConfig.cmake"
    PATH_VARS CMAKE_INSTALL_PREFIX CMAKE_INSTALL_FULL_BINDIR CMAKE_INSTALL_FULL_INCLUDEDIR CMAKE_INSTALL_FULL_LIBDIR
    INSTALL_DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/libOPNMIDI"
)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/libOPNMIDIConfig.cmake
        DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/libOPNMIDI")

if(WITH_EXTRA_BANKS AND NOT APPLE)
    file(GLOB WOPN_FILES ${libOPNMIDI_SOURCE_DIR}/fm_banks/*.wopn)
    install(FILES ${WOPN_FILES}
            DESTINATION "${CMAKE_INSTALL_DATAROOTDIR}/sounds/wopn")

    file(GLOB WOPNTXT_FILES ${libOPNMIDI_SOURCE_DIR}/fm_banks/*.txt)
    install(FILES ${WOPNTXT_FILES}
            DESTINATION "${CMAKE_INSTALL_DATAROOTDIR}/doc/libOPNMIDI/wopn")
endif()

file(GLOB DOCTXT_FILES
    "${libOPNMIDI_SOURCE_DIR}/docs/*.txt"
    "${libOPNMIDI_SOURCE_DIR}/LICENSE*.txt"
    README.md AUTHORS
)
install(FILES ${DOCTXT_FILES}
        DESTINATION "${CMAKE_INSTALL_DATAROOTDIR}/doc/libOPNMIDI")

if(NOT ANDROID)
    configure_file(libOPNMIDI.pc.in libOPNMIDI.pc @ONLY)
    install(FILES "${CMAKE_CURRENT_BINARY_DIR}/libOPNMIDI.pc"
                  DESTINATION "${CMAKE_INSTALL_LIBDIR}/pkgconfig")
endif()

option(WITH_UNIT_TESTS   "Enable unit testing" OFF)
if(WITH_UNIT_TESTS)
    enable_testing ()
    add_subdirectory(test)
endif()

# === Version check ====
set(VERSION_CHECK_SOURCE "
    #include \"opnmidi.h\"
    #if !(OPNMIDI_VERSION_MAJOR == ${PROJECT_VERSION_MAJOR} && OPNMIDI_VERSION_MINOR == ${PROJECT_VERSION_MINOR} && OPNMIDI_VERSION_PATCHLEVEL == ${PROJECT_VERSION_PATCH})
    #error Project and source code version do not match!
    #endif")
file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/version_check.c" "${VERSION_CHECK_SOURCE}")
add_library(OPNMIDI_version_check OBJECT "${CMAKE_CURRENT_BINARY_DIR}/version_check.c")
target_include_directories(OPNMIDI_version_check PRIVATE "include")

message("==== libOPNMIDI options ====")
message("libOPNMIDI_STATIC        = ${libOPNMIDI_STATIC}")
message("libOPNMIDI_SHARED        = ${libOPNMIDI_SHARED}")
message("WITH_UNIT_TESTS          = ${WITH_UNIT_TESTS}")

# message("WITH_CPP_EXTRAS          = ${WITH_CPP_EXTRAS}")
message("WITH_MIDI_SEQUENCER      = ${WITH_MIDI_SEQUENCER}")
message("WITH_HQ_RESAMPLER        = ${WITH_HQ_RESAMPLER}")
message("WITH_MUS_SUPPORT         = ${WITH_MUS_SUPPORT}")
message("WITH_XMI_SUPPORT         = ${WITH_XMI_SUPPORT}")
message("USE_MAME_EMULATOR        = ${USE_MAME_EMULATOR}")
message("USE_GENS_EMULATOR        = ${USE_GENS_EMULATOR}")
message("USE_NUKED_EMULATOR       = ${USE_NUKED_EMULATOR}")
#message("USE_GX_EMULATOR          = ${USE_GX_EMULATOR}")
message("USE_NP2_EMULATOR         = ${USE_NP2_EMULATOR}")
message("USE_MAME_2608_EMULATOR   = ${USE_MAME_2608_EMULATOR}")
#message("USE_PMDWIN_EMULATOR      = ${USE_PMDWIN_EMULATOR}")
message("USE_VGM_FILE_DUMPER      = ${USE_VGM_FILE_DUMPER}")
message("USE_YMFM_EMULATOR        = ${USE_YMFM_EMULATOR}")

message("===== Utils and extras =====")
message("WITH_MIDIPLAY            = ${WITH_MIDIPLAY}")
message("WITH_WOPN2HPP            = ${WITH_WOPN2HPP}")
message("WITH_VLC_PLUGIN          = ${WITH_VLC_PLUGIN}")
message("WITH_DAC_UTIL            = ${WITH_DAC_UTIL}")
if(WIN32)
    message("WITH_WINMMDRV            = ${WITH_WINMMDRV}")
endif()
if(NOT APPLE)
    message("WITH_EXTRA_BANKS         = ${WITH_EXTRA_BANKS}")
endif()
