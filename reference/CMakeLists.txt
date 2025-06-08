cmake_minimum_required(VERSION 3.14)
project(AnimationCHOP VERSION 1.0.0 LANGUAGES CXX)

set(VERBOSE_STATUS ON CACHE BOOL "Verbose status messages" FORCE)
set(SUPPRESS_NOT_REFERENCED_WARNINGS ON CACHE BOOL "Suppress not referenced warnings" FORCE)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(SRC ${CMAKE_CURRENT_SOURCE_DIR}/src)

set(PY_ANIM_BINDINGS_SRCS
    ${SRC}/py_anim_bindings/py_channel.cpp
    ${SRC}/py_anim_bindings/py_function.cpp
    ${SRC}/py_anim_bindings/py_handle_mode.cpp
    ${SRC}/py_anim_bindings/py_keyframe.cpp
    ${SRC}/py_anim_bindings/py_point.cpp
)

# Explicitly define sources for AnimationCHOP
set(ANIMATION_CHOP_SOURCES
    ${PY_ANIM_BINDINGS_SRCS}
    ${SRC}/animation_chop.cpp
)

# Explicitly define sources for AnimationViewCHOP
set(ANIMATION_VIEW_CHOP_SOURCES
    ${SRC}/animation_view_chop.cpp
)

if(VERBOSE_STATUS)
    foreach(SOURCE ${ANIMATION_CHOP_SOURCES})
        message(STATUS "Found AnimationCHOP source file: ${SOURCE}")
    endforeach()
    foreach(SOURCE ${ANIMATION_VIEW_CHOP_SOURCES})
        message(STATUS "Found AnimationViewCHOP source file: ${SOURCE}")
    endforeach()
endif()

# Explicitly disable anim tests and examples when building as submodule
set(ANIM_BUILD_TESTS OFF CACHE BOOL "Build anim tests" FORCE)
set(ANIM_BUILD_EXAMPLES OFF CACHE BOOL "Build anim examples" FORCE)

# Add subdirectory for anim library to build it
add_subdirectory(ext/anim)

# Create the animation_chop target
if(APPLE)
    add_library(animation_chop MODULE ${ANIMATION_CHOP_SOURCES})
else()
    add_library(animation_chop SHARED ${ANIMATION_CHOP_SOURCES})
endif()

# Create the animation_view_chop target
if(APPLE)
    add_library(animation_view_chop MODULE ${ANIMATION_VIEW_CHOP_SOURCES})
else()
    add_library(animation_view_chop SHARED ${ANIMATION_VIEW_CHOP_SOURCES})
endif()

# Include directories for both targets
foreach(TARGET animation_chop animation_view_chop)
    target_include_directories(${TARGET} PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${CMAKE_CURRENT_SOURCE_DIR}/ext/td/include
        ${CMAKE_CURRENT_SOURCE_DIR}/ext/Python/Include
        ${CMAKE_CURRENT_SOURCE_DIR}/ext/anim/include
    )
endforeach()

if(WIN32)
    foreach(TARGET animation_chop animation_view_chop)
        target_include_directories(${TARGET} PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/ext/Python/Include/PC
        )

        target_link_directories(${TARGET} PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/ext/Python/lib/x64
        )

        # Both targets now need Python for their bindings
        target_link_libraries(${TARGET} PRIVATE 
            python311
            anim
        )
    endforeach()
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
        
        foreach(TARGET animation_chop animation_view_chop)
            target_link_libraries(${TARGET} PRIVATE 
                Python3::Python
                anim
            )
            target_compile_definitions(${TARGET} PRIVATE __cdecl=)
        endforeach()
    else()
        message(WARNING "TouchDesigner's Python not found at ${TD_PYTHON_EXECUTABLE}, falling back to system Python")
        find_package(Python3 COMPONENTS Interpreter Development REQUIRED)
        foreach(TARGET animation_chop animation_view_chop)
            target_link_libraries(${TARGET} PRIVATE 
                Python3::Python
                anim
            )
            target_compile_definitions(${TARGET} PRIVATE __cdecl=)
        endforeach()
    endif()
else()
    # On other platforms, use pkg-config to find Python
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(PYTHON REQUIRED python3)
    foreach(TARGET animation_chop animation_view_chop)
        target_include_directories(${TARGET} PRIVATE ${PYTHON_INCLUDE_DIRS})
        target_link_libraries(${TARGET} PRIVATE 
            ${PYTHON_LIBRARIES}
            anim
        )
        target_compile_definitions(${TARGET} PRIVATE __cdecl=)
    endforeach()
endif()

# Define output directories for both targets
set_target_properties(animation_chop PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    OUTPUT_NAME "AnimationCHOP"
    PREFIX ""
)

set_target_properties(animation_view_chop PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    OUTPUT_NAME "AnimationViewCHOP"
    PREFIX ""
)

if(SUPPRESS_NOT_REFERENCED_WARNINGS)
    foreach(TARGET animation_chop animation_view_chop)
        if(MSVC)
            target_compile_options(${TARGET} PRIVATE /wd4100 /wd4189)
            target_compile_definitions(${TARGET} PRIVATE _CRT_SECURE_NO_WARNINGS)
        endif()
        if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
            target_compile_options(${TARGET} PRIVATE -Wno-unused-parameter -Wno-unused-variable)  
        endif()
    endforeach()
endif()

# Add post-build commands to copy both DLLs to the Plugins folder
foreach(TARGET animation_chop animation_view_chop)
    add_custom_command(TARGET ${TARGET} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_SOURCE_DIR}/td/Plugins"
        COMMENT "Creating Plugins directory"
    )
    
    if(APPLE)
        # On macOS, copy the entire .plugin bundle
        add_custom_command(TARGET ${TARGET} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_directory 
                    "$<TARGET_BUNDLE_DIR:${TARGET}>" 
                    "${CMAKE_SOURCE_DIR}/td/Plugins/$<TARGET_PROPERTY:${TARGET},OUTPUT_NAME>.plugin"
            COMMENT "Copying ${TARGET}.plugin bundle to Plugins directory"
        )
    else()
        # On other platforms, copy the library file
        add_custom_command(TARGET ${TARGET} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy 
                    "$<TARGET_FILE:${TARGET}>" 
                    "${CMAKE_SOURCE_DIR}/td/Plugins/"
            COMMENT "Copying ${TARGET} to Plugins directory"
        )
    endif()
endforeach()

# Add platform-specific settings for both targets
foreach(TARGET animation_chop animation_view_chop)
    if(WIN32)
        target_compile_definitions(${TARGET} PRIVATE WIN32 _WINDOWS _USRDLL)
        # Windows-specific settings
        if(MSVC)
            # MSVC compiler flags
            target_compile_options(${TARGET} PRIVATE /W4)
        endif()
    elseif(APPLE)
        # macOS-specific settings for TouchDesigner modules
        string(REPLACE "_" "-" BUNDLE_ID_TARGET ${TARGET})
        set_target_properties(${TARGET} PROPERTIES
            BUNDLE TRUE
            BUNDLE_EXTENSION "plugin"
            MACOSX_BUNDLE_GUI_IDENTIFIER "com.actualize.${BUNDLE_ID_TARGET}"
            MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
            MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION}
            MACOSX_BUNDLE_INFO_PLIST "${CMAKE_CURRENT_SOURCE_DIR}/Info.plist.in"
            XCODE_ATTRIBUTE_PRODUCT_BUNDLE_IDENTIFIER "com.actualize.${BUNDLE_ID_TARGET}"
        )
    elseif(UNIX)
        # Linux-specific settings
        target_compile_options(${TARGET} PRIVATE -Wall -Wextra)
    endif()
endforeach()

# Install targets
install(TARGETS animation_chop animation_view_chop
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)
