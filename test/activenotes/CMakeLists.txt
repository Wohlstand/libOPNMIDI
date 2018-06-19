
set(CMAKE_CXX_STANDARD 11)

include_directories (${CMAKE_CURRENT_SOURCE_DIR}/../common
                     ${CMAKE_SOURCE_DIR}/include
                     ${CMAKE_SOURCE_DIR}/src)

add_executable (ActiveNotesList
                active_notes.cpp
                $<TARGET_OBJECTS:Catch-objects>)

set_target_properties(ActiveNotesList PROPERTIES COMPILE_DEFINITIONS "GSL_THROW_ON_CONTRACT_VIOLATION")
target_link_libraries(ActiveNotesList PRIVATE OPNMIDI_static)
target_compile_definitions(ActiveNotesList PRIVATE OPNMIDI_AUDIO_TICK_HANDLER)
add_test(NAME ActiveNotesList COMMAND ActiveNotesList)
