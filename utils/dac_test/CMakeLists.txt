include(${CMAKE_CURRENT_LIST_DIR}/../../cmake/libSDL2.cmake)

add_executable(dac_test
    dac_test.cpp
    ${libOPNMIDI_SOURCE_DIR}/src/chips/nuked_opn2.cpp
    ${libOPNMIDI_SOURCE_DIR}/src/chips/nuked/ym3438.c
)

target_include_directories(dac_test PRIVATE
        ${libOPNMIDI_SOURCE_DIR}/src/chips/
)

target_link_libraries(dac_test ${SDL2_NEEDED_LIBRARIES})

install(TARGETS dac_test
        RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
        LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
        ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
        INCLUDES DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}")

if(WITH_HQ_RESAMPLER)
    target_link_libraries(dac_test "${ZITA_RESAMPLER_LIBRARY}")
endif()
