cmake_minimum_required(VERSION 3.24)
project(vzlet-z256 VERSION 0.1.0 LANGUAGES C CXX)

option(VZ256_BUILD_APP "Build the SDL3 emulator front end" ON)
option(VZ256_BUILD_TESTS "Build unit tests" ON)

add_library(vz256_core
    src/machine.cpp
    src/video.cpp
    src/floppy.cpp)
target_include_directories(vz256_core PUBLIC include)
target_compile_features(vz256_core PUBLIC cxx_std_20)
target_compile_options(vz256_core PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Wconversion>)

if(VZ256_BUILD_APP)
    include(FetchContent)
    find_package(SDL3 3.2 QUIET CONFIG)
    if(NOT SDL3_FOUND)
        message(FATAL_ERROR "SDL3 was not found. Install SDL3 or configure with -DVZ256_BUILD_APP=OFF")
    endif()

    # The CPU core is intentionally obtained from the project requested by the
    # hardware specification.  VZ256_Z80_SOURCE_DIR permits reproducible/offline builds.
    set(VZ256_Z80_SOURCE_DIR "" CACHE PATH "Existing checkout of redcode/Z80")
    if(VZ256_Z80_SOURCE_DIR)
        set(z80_SOURCE_DIR "${VZ256_Z80_SOURCE_DIR}")
    else()
        FetchContent_Declare(z80
            GIT_REPOSITORY https://github.com/redcode/Z80.git
            GIT_TAG master
            GIT_SHALLOW TRUE)
        FetchContent_MakeAvailable(z80)
    endif()

    find_path(Z80_INCLUDE_DIR Z80.h
        HINTS "${z80_SOURCE_DIR}" "${z80_SOURCE_DIR}/sources" "${z80_SOURCE_DIR}/include")
    find_file(Z80_SOURCE_FILE NAMES Z80.c z80.c
        HINTS "${z80_SOURCE_DIR}" "${z80_SOURCE_DIR}/sources" "${z80_SOURCE_DIR}/source")
    if(NOT Z80_INCLUDE_DIR OR NOT Z80_SOURCE_FILE)
        message(FATAL_ERROR "The redcode/Z80 checkout does not contain Z80.h and Z80.c")
    endif()

    add_library(redcode_z80 STATIC "${Z80_SOURCE_FILE}")
    target_include_directories(redcode_z80 PUBLIC "${Z80_INCLUDE_DIR}")

    add_executable(vz256 src/main.cpp src/redcode_cpu.cpp)
    target_link_libraries(vz256 PRIVATE vz256_core redcode_z80 SDL3::SDL3)
    target_compile_features(vz256 PRIVATE cxx_std_20)
endif()

if(VZ256_BUILD_TESTS)
    enable_testing()
    add_executable(vz256_tests tests/core_tests.cpp)
    target_link_libraries(vz256_tests PRIVATE vz256_core)
    add_test(NAME core COMMAND vz256_tests)
endif()
