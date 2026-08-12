cmake_minimum_required(VERSION 3.20)
project(vzlet-z256 VERSION 0.1.0 LANGUAGES CXX)

option(VZ256_WITH_SDL "Build the SDL3 front end" ON)
option(VZ256_BUILD_TESTS "Build unit tests" ON)

add_library(vz256_core
    src/machine.cpp
    src/wd2797.cpp
)
target_include_directories(vz256_core PUBLIC include)
target_compile_features(vz256_core PUBLIC cxx_std_20)
target_compile_options(vz256_core PRIVATE -Wall -Wextra -Wpedantic)

if(VZ256_WITH_SDL)
  find_package(SDL3 CONFIG QUIET)
  if(SDL3_FOUND)
    add_executable(vz256 src/main.cpp)
    target_link_libraries(vz256 PRIVATE vz256_core SDL3::SDL3)
    target_compile_definitions(vz256 PRIVATE VZ256_DEFAULT_DATA_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
  else()
    message(WARNING "SDL3 was not found; only the headless core will be built")
  endif()
endif()

if(VZ256_BUILD_TESTS)
  enable_testing()
  add_executable(vz256_tests tests/machine_test.cpp)
  target_link_libraries(vz256_tests PRIVATE vz256_core)
  target_compile_definitions(vz256_tests PRIVATE SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
  add_test(NAME vz256_tests COMMAND vz256_tests)
endif()
