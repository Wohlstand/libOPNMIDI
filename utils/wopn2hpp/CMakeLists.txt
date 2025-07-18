add_executable(wopn2hpp wopn2hpp.c)
install(TARGETS wopn2hpp RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}")

if(WIN32)
    set_property(TARGET wopn2hpp PROPERTY WIN32_EXECUTABLE OFF)
endif()
