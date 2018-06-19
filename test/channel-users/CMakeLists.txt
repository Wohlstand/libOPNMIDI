
set(CMAKE_CXX_STANDARD 11)

include_directories (${CMAKE_CURRENT_SOURCE_DIR}/../common
                     ${CMAKE_SOURCE_DIR}/include
                     ${CMAKE_SOURCE_DIR}/src)

add_executable(ChannelUsersTest
               channel_users.cpp
               $<TARGET_OBJECTS:Catch-objects>)

set_target_properties(ChannelUsersTest PROPERTIES COMPILE_DEFINITIONS "GSL_THROW_ON_CONTRACT_VIOLATION")
target_link_libraries(ChannelUsersTest PRIVATE OPNMIDI_static)
target_compile_definitions(ChannelUsersTest PRIVATE OPNMIDI_AUDIO_TICK_HANDLER)
add_test(NAME ChannelUsersTest COMMAND ChannelUsersTest)

