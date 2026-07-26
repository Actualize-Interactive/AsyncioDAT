cmake_minimum_required(VERSION 3.25)
project(AsyncioDAT VERSION 0.4.0 LANGUAGES CXX)

set(VERBOSE_STATUS OFF CACHE BOOL "Verbose status messages" FORCE)
set(SUPPRESS_NOT_REFERENCED_WARNINGS ON CACHE BOOL "Suppress not referenced warnings" FORCE)

set(CMAKE_CXX_STANDARD 20)
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

# --- Python ----------------------------------------------------------------
# TouchDesigner embeds CPython 3.11 and supplies python311 at runtime, so we do
# NOT vendor Python headers/libs. Provide them via a uv-managed interpreter:
#
#   uv python install 3.11
#   cmake --preset dev      # CMake auto-detects the uv Python 3.11
#
# (A uv-managed CPython ships full headers + import lib in its base prefix; a
# bare venv does not, since venvs only reference the base interpreter.)
#
# We request the Development.Module component only: on Windows it links the
# python311 import library; on macOS it builds a loadable module whose Python
# symbols resolve against TouchDesigner's interpreter at load time (so no
# hardcoded framework path is needed).
#
# Resolution order:
#   1. -DPython3_ROOT_DIR=... or -DASYNCIODAT_PYTHON_ROOT=... if you pass one
#   2. otherwise, auto-detect a uv-managed 3.11 via `uv python find 3.11`
#   3. otherwise, CMake's normal search (must still be 3.11)
if(DEFINED ASYNCIODAT_PYTHON_ROOT)
    set(Python3_ROOT_DIR "${ASYNCIODAT_PYTHON_ROOT}")
endif()

find_program(UV_EXECUTABLE uv)

if(NOT DEFINED Python3_ROOT_DIR AND UV_EXECUTABLE)
    execute_process(
        COMMAND "${UV_EXECUTABLE}" python find 3.11
        OUTPUT_VARIABLE _uv_python
        OUTPUT_STRIP_TRAILING_WHITESPACE
        RESULT_VARIABLE _uv_result
        ERROR_QUIET)
    if(_uv_result EQUAL 0 AND EXISTS "${_uv_python}")
        get_filename_component(_uv_dir "${_uv_python}" DIRECTORY)
        get_filename_component(_uv_dir_name "${_uv_dir}" NAME)
        # Windows: <root>/python.exe; POSIX: <root>/bin/python3.11
        if(_uv_dir_name STREQUAL "bin")
            get_filename_component(_uv_dir "${_uv_dir}" DIRECTORY)
        endif()
        set(Python3_ROOT_DIR "${_uv_dir}")
        set(ASYNCIODAT_PY_VIA_UV ON)
        message(STATUS "AsyncioDAT: using uv-managed Python at ${Python3_ROOT_DIR}")
    endif()
endif()

set(Python3_FIND_STRATEGY LOCATION)
find_package(Python3 3.11 COMPONENTS Development.Module REQUIRED)

if(NOT Python3_VERSION MATCHES "^3\\.11")
    message(FATAL_ERROR
        "TouchDesigner requires Python 3.11, but found ${Python3_VERSION} at "
        "${Python3_EXECUTABLE}. Install it with 'uv python install 3.11' and "
        "reconfigure with -DPython3_ROOT_DIR=<uv python 3.11 dir>.")
endif()
message(STATUS "AsyncioDAT: using Python ${Python3_VERSION} (${Python3_INCLUDE_DIRS})")

# recursively get all .cpp files in the src directory
file(GLOB_RECURSE SOURCES
    src/*.cpp
)

if(VERBOSE_STATUS)
    foreach(SOURCE ${SOURCES})
        message(STATUS "Found source file: ${SOURCE}")
    endforeach()
endif()

# Create the target with proper library type for each platform
if(APPLE)
    add_library(asyncio_dat MODULE ${SOURCES})
else()
    add_library(asyncio_dat SHARED ${SOURCES})
endif()

# Include directories
target_include_directories(asyncio_dat PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/ext/td/include
)

# Link Python (via Development.Module, found above) and toml11 on all platforms.
target_link_libraries(asyncio_dat PRIVATE
    Python3::Module
    toml11::toml11
)

# TouchDesigner's SDK headers use __cdecl annotations that are only meaningful
# on MSVC; neutralize them elsewhere.
if(NOT MSVC)
    target_compile_definitions(asyncio_dat PRIVATE __cdecl=)
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

# Add post-build command to copy the library to the Plugins folder
add_custom_command(TARGET asyncio_dat POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_SOURCE_DIR}/tests/td/Plugins"
    COMMENT "Creating Plugins directory"
)

if(APPLE)
    # On macOS, copy the entire .plugin bundle
    add_custom_command(TARGET asyncio_dat POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory 
                "$<TARGET_BUNDLE_DIR:asyncio_dat>" 
                "${CMAKE_SOURCE_DIR}/tests/td/Plugins/$<TARGET_PROPERTY:asyncio_dat,OUTPUT_NAME>.plugin"
        COMMENT "Copying AsyncioDAT.plugin bundle to Plugins directory"
    )
else()
    # On other platforms, copy the library file
    add_custom_command(TARGET asyncio_dat POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy 
                "$<TARGET_FILE:asyncio_dat>" 
                "${CMAKE_SOURCE_DIR}/tests/td/Plugins/"
        COMMENT "Copying AsyncioDAT to Plugins directory"
    )
endif()

# Add platform-specific settings
if(WIN32)
    target_compile_definitions(asyncio_dat PRIVATE WIN32 _WINDOWS _USRDLL)
    # Windows-specific settings
    if(MSVC)
        # MSVC compiler flags
        target_compile_options(asyncio_dat PRIVATE /W4)
    endif()
elseif(APPLE)
    # macOS-specific settings for TouchDesigner modules
    set_target_properties(asyncio_dat PROPERTIES
        BUNDLE TRUE
        BUNDLE_EXTENSION "plugin"
        MACOSX_BUNDLE_GUI_IDENTIFIER "com.actualize.asyncio-dat"
        MACOSX_BUNDLE_BUNDLE_NAME "AsyncioDAT"
        MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
        MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION}
        MACOSX_BUNDLE_INFO_PLIST "${CMAKE_CURRENT_SOURCE_DIR}/Info.plist.in"
        XCODE_ATTRIBUTE_PRODUCT_BUNDLE_IDENTIFIER "com.actualize.asyncio-dat"
    )
elseif(UNIX)
    # Linux-specific settings
    target_compile_options(asyncio_dat PRIVATE -Wall -Wextra)
endif()

# Install targets
install(TARGETS asyncio_dat
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)

# --- Unit tests ------------------------------------------------------------
# tests/cpp  : Catch2 C++ tests (no Python or TouchDesigner needed)
# tests/python: a CPython extension exposing the bindings + a pytest suite
# Neither needs TouchDesigner. Configure with -DASYNCIODAT_BUILD_TESTS=ON,
# build, then run with ctest (pytest must be installed in the interpreter).
option(ASYNCIODAT_BUILD_TESTS "Build unit tests (Catch2 + pytest extension)" OFF)
if(ASYNCIODAT_BUILD_TESTS)
    enable_testing()
    # The pytest suite needs an interpreter; the extension needs Development.Module.
    find_package(Python3 3.11 COMPONENTS Interpreter Development.Module REQUIRED)
    add_subdirectory(tests/cpp)
    add_subdirectory(tests/python)
endif()