cmake_minimum_required(VERSION 3.25)
project(AsyncioDAT VERSION 1.0.0 LANGUAGES CXX)

set(VERBOSE_STATUS OFF CACHE BOOL "Verbose status messages" FORCE)
set(SUPPRESS_NOT_REFERENCED_WARNINGS ON CACHE BOOL "Suppress not referenced warnings" FORCE)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Add toml11 library
include(FetchContent)
FetchContent_Declare(
  toml11
  GIT_REPOSITORY https://github.com/ToruNiina/toml11.git
  GIT_TAG v3.8.1
)
FetchContent_MakeAvailable(toml11)

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
)

# Add platform-specific definitions
if(NOT WIN32)
    target_compile_definitions(asyncio_dat PRIVATE __cdecl=)
endif()

# Add Windows-specific include directory if on Windows
if(WIN32)
    target_include_directories(asyncio_dat PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/ext/Python/Include/PC
    )
endif()

# Find Python installation for linking
if(WIN32)
    # On Windows, link to the included Python libraries in ext/Python
    target_link_directories(asyncio_dat PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/ext/Python/lib/x64
    )
    
    # Link against the included Python library
    target_link_libraries(asyncio_dat PRIVATE 
        python311
        toml11::toml11
    )
elseif(APPLE)
    # On macOS, use the system Python or the one set up by GitHub Actions
    find_package(Python3 COMPONENTS Interpreter Development REQUIRED)
    target_link_libraries(asyncio_dat PRIVATE Python3::Python toml11::toml11)
else()
    # On other platforms, use pkg-config to find Python
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(PYTHON REQUIRED python3)
    target_include_directories(asyncio_dat PRIVATE ${PYTHON_INCLUDE_DIRS})
    target_link_libraries(asyncio_dat PRIVATE ${PYTHON_LIBRARIES} toml11::toml11)
endif()

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
        target_compile_definitions(asyncio_dat PRIVATE _CRT_SECURE_NO_WARNINGS)
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
        MACOSX_BUNDLE_GUI_IDENTIFIER com.actualize.asynciodat
    )
elseif(UNIX)
    # Linux-specific settings
    target_compile_options(asyncio_dat PRIVATE -Wall -Wextra -Wno-changes-meaning -fpermissive)
endif()

