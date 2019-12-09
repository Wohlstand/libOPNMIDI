find_package(SDL2 REQUIRED)
string(STRIP ${SDL2_LIBRARIES} SDL2_LIBRARIES)

add_executable(opnmidi_sdl2_demo sdl2_sample.c)
target_include_directories(opnmidi_sdl2_demo PRIVATE ${SDL2_INCLUDE_DIRS})
target_link_libraries(opnmidi_sdl2_demo PRIVATE OPNMIDI ${SDL2_LIBRARIES})

set_nopie(opnmidi_sdl2_demo)

if(NOT MSVC)
    target_link_libraries(opnmidi_sdl2_demo PRIVATE pthread)
endif()
if(NOT WIN32)
    target_link_libraries(opnmidi_sdl2_demo PRIVATE dl m)
endif()

if(TARGET opnmidi_shared)
    set_target_properties(opnmidi_sdl2_demo PROPERTIES INSTALL_RPATH "$ORIGIN/../lib")
endif()

install(TARGETS opnmidi_sdl2_demo
        RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}")

