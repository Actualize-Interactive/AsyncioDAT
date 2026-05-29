# Python extension + pytest suite. Exposes the operator bindings without
# TouchDesigner by compiling the real operator sources against a fake OP_Context.

Python3_add_library(asynciodat_testext MODULE
    extension/asynciodat_module.cpp
    ${CMAKE_SOURCE_DIR}/src/asyncio_dat.cpp
    ${CMAKE_SOURCE_DIR}/src/py_bindings.cpp
    ${CMAKE_SOURCE_DIR}/src/config.cpp
)
set_target_properties(asynciodat_testext PROPERTIES OUTPUT_NAME "asynciodat" PREFIX "")
target_include_directories(asynciodat_testext PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/ext/td/include
)
target_link_libraries(asynciodat_testext PRIVATE Python3::Module toml11::toml11)
target_compile_features(asynciodat_testext PRIVATE cxx_std_20)

if(MSVC)
    target_compile_options(asynciodat_testext PRIVATE /wd4100 /wd4189)
    target_compile_definitions(asynciodat_testext PRIVATE _CRT_SECURE_NO_WARNINGS)
else()
    target_compile_definitions(asynciodat_testext PRIVATE __cdecl=)
endif()

# Copy the built module next to the pytest files so `import asynciodat` works
# regardless of build-config subdirectories.
add_custom_command(TARGET asynciodat_testext POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
            "$<TARGET_FILE:asynciodat_testext>"
            "${CMAKE_CURRENT_SOURCE_DIR}/"
    COMMENT "Copying asynciodat test extension to tests/python/"
)

# Run the pytest suite as a ctest test (requires pytest in the interpreter).
add_test(
    NAME python_bindings
    COMMAND ${Python3_EXECUTABLE} -m pytest -q "${CMAKE_CURRENT_SOURCE_DIR}"
)
set_tests_properties(python_bindings PROPERTIES
    DEPENDS asynciodat_testext
)
