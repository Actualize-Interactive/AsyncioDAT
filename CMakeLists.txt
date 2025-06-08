cmake_minimum_required(VERSION 3.25)
project(AsyncioDAT VERSION 1.0.0 LANGUAGES CXX)

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
    ${CMAKE_CURRENT_SOURCE_DIR}/ext/Python/Include
)

# Add Windows-specific include directory if on Windows
if(WIN32)
    target_include_directories(asyncio_dat PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/ext/Python/Include/PC
    )

    target_link_directories(asyncio_dat PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/ext/Python/lib/x64
    )

    target_link_libraries(asyncio_dat PRIVATE 
        python311
        toml11::toml11
    )
elseif(APPLE)
    # On macOS, use TouchDesigner's Python specifically
    set(TD_PYTHON_EXECUTABLE "/Applications/TouchDesigner.app/Contents/Frameworks/Python.framework/Versions/Current/bin/python3.11")
    set(TD_PYTHON_INCLUDE_DIR "/Applications/TouchDesigner.app/Contents/Frameworks/Python.framework/Versions/Current/include/python3.11")
    set(TD_PYTHON_LIBRARY_DIR "/Applications/TouchDesigner.app/Contents/Frameworks/Python.framework/Versions/Current/lib")
    
    # Check if TouchDesigner's Python exists, otherwise fall back to system Python
    if(EXISTS "${TD_PYTHON_EXECUTABLE}" AND EXISTS "${TD_PYTHON_INCLUDE_DIR}")
        message(STATUS "Using TouchDesigner's Python: ${TD_PYTHON_EXECUTABLE}")
        
        # Force CMake to use TouchDesigner's Python by setting the paths explicitly
        set(Python3_EXECUTABLE "${TD_PYTHON_EXECUTABLE}" CACHE FILEPATH "Path to Python3 executable" FORCE)
        set(Python3_INCLUDE_DIR "${TD_PYTHON_INCLUDE_DIR}" CACHE PATH "Path to Python3 include directory" FORCE)
        set(Python3_LIBRARY_DIRS "${TD_PYTHON_LIBRARY_DIR}" CACHE PATH "Path to Python3 library directory" FORCE)
        
        # Set the find strategy to prioritize our specified paths
        set(Python3_FIND_STRATEGY LOCATION)
        set(Python3_FIND_REGISTRY NEVER)
        set(Python3_FIND_FRAMEWORK NEVER)
        
        find_package(Python3 COMPONENTS Interpreter Development REQUIRED)
        
        target_link_libraries(asyncio_dat PRIVATE 
            Python3::Python
            toml11::toml11
        )
        target_compile_definitions(asyncio_dat PRIVATE __cdecl=)
    else()
        message(WARNING "TouchDesigner's Python not found at ${TD_PYTHON_EXECUTABLE}, falling back to system Python")
        find_package(Python3 COMPONENTS Interpreter Development REQUIRED)
        target_link_libraries(asyncio_dat PRIVATE 
            Python3::Python
            toml11::toml11
        )
        target_compile_definitions(asyncio_dat PRIVATE __cdecl=)
    endif()
else()
    # On other platforms, use pkg-config to find Python
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(PYTHON REQUIRED python3)
    target_include_directories(asyncio_dat PRIVATE ${PYTHON_INCLUDE_DIRS})
    target_link_libraries(asyncio_dat PRIVATE 
        ${PYTHON_LIBRARIES}
        toml11::toml11
    )
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
    COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_SOURCE_DIR}/test/Plugins"
    COMMENT "Creating Plugins directory"
)

if(APPLE)
    # On macOS, copy the entire .plugin bundle
    add_custom_command(TARGET asyncio_dat POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory 
                "$<TARGET_BUNDLE_DIR:asyncio_dat>" 
                "${CMAKE_SOURCE_DIR}/test/Plugins/$<TARGET_PROPERTY:asyncio_dat,OUTPUT_NAME>.plugin"
        COMMENT "Copying AsyncioDAT.plugin bundle to Plugins directory"
    )
else()
    # On other platforms, copy the library file
    add_custom_command(TARGET asyncio_dat POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy 
                "$<TARGET_FILE:asyncio_dat>" 
                "${CMAKE_SOURCE_DIR}/test/Plugins/"
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