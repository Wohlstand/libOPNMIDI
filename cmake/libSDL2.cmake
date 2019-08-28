
find_library(SDL2_LIBRARY SDL2 REQUIRED)
include_directories(${SDL2_INCLUDE_DIR})
message("Found ${SDL2_LIBRARY}")

set(SDL2_NEEDED_LIBRARIES)
if(WIN32)
    if(MSVC)
        list(APPEND SDL2_NEEDED_LIBRARIES ${SDL2_LIBRARY})
    else()
        list(APPEND SDL2_NEEDED_LIBRARIES ${SDL2_LIBRARY} pthread)
    endif()
else()
    list(APPEND SDL2_NEEDED_LIBRARIES ${SDL2_LIBRARY} pthread)
    if(NOT HAIKU AND NOT OPENBSD_LOCALBASE)
        list(APPEND SDL2_NEEDED_LIBRARIES dl)
    endif()
    list(APPEND SDL2_NEEDED_LIBRARIES m stdc++)
endif()

if(OPENBSD_LOCALBASE)
    list(REMOVE_ITEM SDL2_NEEDED_LIBRARIES dl)
endif()

