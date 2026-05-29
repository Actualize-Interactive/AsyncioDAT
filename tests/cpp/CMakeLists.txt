# C++ unit tests (Catch2). No Python or TouchDesigner required.
include(FetchContent)
FetchContent_Declare(
  Catch2
  GIT_REPOSITORY https://github.com/catchorg/Catch2.git
  GIT_TAG v3.5.2
)
FetchContent_MakeAvailable(Catch2)

add_executable(asyncio_dat_cpp_tests
  test_config.cpp
  ${CMAKE_SOURCE_DIR}/src/config.cpp
)
target_include_directories(asyncio_dat_cpp_tests PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(asyncio_dat_cpp_tests PRIVATE
  Catch2::Catch2WithMain
  toml11::toml11
)
target_compile_features(asyncio_dat_cpp_tests PRIVATE cxx_std_20)

list(APPEND CMAKE_MODULE_PATH ${catch2_SOURCE_DIR}/extras)
include(Catch)
catch_discover_tests(asyncio_dat_cpp_tests)
