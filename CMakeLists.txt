cmake_minimum_required(VERSION 3.25)
project(AnimationCHOP VERSION 1.0.0 LANGUAGES CXX)

set(VERBOSE_STATUS OFF CACHE BOOL "Verbose status messages" FORCE)
set(SUPPRESS_NOT_REFERENCED_WARNINGS ON CACHE BOOL "Suppress not referenced warnings" FORCE)


set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# recursively get all .cpp files in the src directory
file(GLOB_RECURSE SOURCES
    src/*.cpp
)

if(VERBOSE_STATUS)
    foreach(SOURCE ${SOURCES})
        message(STATUS "Found source file: ${SOURCE}")
    endforeach()
endif()

add_library(asyncio_dat SHARED ${SOURCES})

# Include directories
target_include_directories(asyncio_dat PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/ext/td/include
    ${CMAKE_CURRENT_SOURCE_DIR}/ext/Python/Include
    ${CMAKE_CURRENT_SOURCE_DIR}/ext/Python/Include/PC
)

target_link_directories(asyncio_dat PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/ext/Python/lib/x64
)

# Link against the anim library
target_link_libraries(asyncio_dat PRIVATE 
    python311
)

# Define output directories
set_target_properties(asyncio_dat PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    OUTPUT_NAME "AsyncioDAT"
    PREFIX ""
)

if(SUPPRESS_NOT_REFERENCED_WARNINGS)
    if(MSVC)
        target_compile_options(asyncio_dat PRIVATE /wd4100 /wd4189)
    endif()
    if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        target_compile_options(asyncio_dat PRIVATE -Wno-unused-parameter -Wno-unused-variable)  
    endif()
endif()

# Add a post-build command to copy the DLL to the Plugins folder
add_custom_command(TARGET asyncio_dat POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_SOURCE_DIR}/test/Plugins"
    COMMAND ${CMAKE_COMMAND} -E copy 
            "$<TARGET_FILE:asyncio_dat>" 
            "${CMAKE_SOURCE_DIR}/test/Plugins/"
    COMMENT "Copying AsyncioDAT.dll to Plugins directory"
)

# Add platform-specific settings
if(WIN32)
    target_compile_definitions(asyncio_dat PRIVATE WIN32 _WINDOWS _USRDLL)
    # Windows-specific settings
    if(MSVC)
        # MSVC compiler flags
        target_compile_options(asyncio_dat PRIVATE /W4)
    endif()
elseif(APPLE)
    # macOS-specific settings
    set_target_properties(asyncio_dat PROPERTIES
        BUNDLE TRUE
        MACOSX_BUNDLE_GUI_IDENTIFIER com.actualize.animationchop
    )
elseif(UNIX)
    # Linux-specific settings
    target_compile_options(asyncio_dat PRIVATE -Wall -Wextra)
endif()

