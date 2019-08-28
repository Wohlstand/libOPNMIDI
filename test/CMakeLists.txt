
set(CMAKE_CXX_STANDARD 11)

# Remove HQ resample necessarity (othewise, link will fail)
remove_definitions(-DOPNMIDI_ENABLE_HQ_RESAMPLER)
# Remove VGM File dumper
remove_definitions(-DOPNMIDI_MIDI2VGM)

add_subdirectory(activenotes)
add_subdirectory(channel-users)
add_subdirectory(wopn-file)

add_library(Catch-objects OBJECT "common/catch_main.cpp")
target_include_directories(Catch-objects PRIVATE "common")
