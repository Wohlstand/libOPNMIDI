
set(CMAKE_CXX_STANDARD 11)

include_directories (${CMAKE_CURRENT_SOURCE_DIR}/../common
                     ${CMAKE_SOURCE_DIR}/include
                     ${CMAKE_SOURCE_DIR}/src)

add_executable (ActiveNotesList
                active_notes.cpp
                )

set_target_properties(ActiveNotesList PROPERTIES COMPILE_DEFINITIONS "GSL_THROW_ON_CONTRACT_VIOLATION")
target_link_libraries(ActiveNotesList INTERFACE OPNMIDI_static)
add_test (NAME ActiveNotesList COMMAND PtrListValidator)

