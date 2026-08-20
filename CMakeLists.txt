cmake_minimum_required(VERSION 3.24)
project(vzlet-z256 VERSION 0.1.0 LANGUAGES C CXX)

option(VZ256_BUILD_APP "Build the SDL3 emulator front end" ON)
option(VZ256_BUILD_TESTS "Build unit tests" ON)
option(VZ256_FETCH_SDL3 "Download SDL3 when no system package is available" ON)

add_library(vz256_core
    src/machine.cpp
    src/video.cpp
    src/floppy.cpp
    src/wd2797.cpp)
target_include_directories(vz256_core PUBLIC include)
target_compile_features(vz256_core PUBLIC cxx_std_20)
target_compile_options(vz256_core PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Wconversion>)

if(VZ256_BUILD_APP)
    include(FetchContent)
    find_package(SDL3 3.2 QUIET CONFIG)
    if(NOT SDL3_FOUND AND VZ256_FETCH_SDL3)
        FetchContent_Declare(SDL3
            GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
            GIT_TAG a96677bdf6b4acb84af4ec294e5f60a4e8cbbe03 # release-3.2.22
            GIT_SHALLOW TRUE)
        set(SDL_TEST_LIBRARY OFF CACHE BOOL "" FORCE)
        set(SDL_TESTS OFF CACHE BOOL "" FORCE)
        FetchContent_MakeAvailable(SDL3)
    elseif(NOT SDL3_FOUND)
        message(FATAL_ERROR "SDL3 was not found. Install SDL3 or configure with -DVZ256_BUILD_APP=OFF")
    endif()

    # Pin both upstream projects: redcode/Z80 uses the Zeta portability library.
    FetchContent_Declare(zeta
        GIT_REPOSITORY https://github.com/redcode/Zeta.git
        GIT_TAG 93ba5ab967eef00f074d21bb760fe9dc48afd2d3)
    FetchContent_MakeAvailable(zeta)

    set(VZ256_Z80_SOURCE_DIR "" CACHE PATH "Existing checkout of redcode/Z80")
    if(VZ256_Z80_SOURCE_DIR)
        FetchContent_Declare(z80 SOURCE_DIR "${VZ256_Z80_SOURCE_DIR}")
    else()
        FetchContent_Declare(z80
            GIT_REPOSITORY https://github.com/redcode/Z80.git
            GIT_TAG 91664e84bc90e5c0b2263877aa480c93712f8976)
    endif()
    set(Z80_SHARED_LIBS OFF CACHE BOOL "" FORCE)
    set(Z80_WITH_EXECUTE ON CACHE BOOL "" FORCE)
    set(Z80_WITH_Q ON CACHE BOOL "" FORCE)
    set(Z80_WITH_ZILOG_NMOS_LD_A_IR_BUG ON CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(z80)

    add_executable(vz256 src/main.cpp src/redcode_cpu.cpp)
    target_link_libraries(vz256 PRIVATE vz256_core Z80 SDL3::SDL3)
    target_compile_features(vz256 PRIVATE cxx_std_20)
endif()

if(VZ256_BUILD_TESTS)
    enable_testing()
    add_executable(vz256_tests tests/core_tests.cpp)
    target_link_libraries(vz256_tests PRIVATE vz256_core)
    target_compile_options(vz256_tests PRIVATE $<$<CXX_COMPILER_ID:GNU,Clang>:-UNDEBUG>)
    add_test(NAME core COMMAND vz256_tests)
    if(VZ256_BUILD_APP)
        add_executable(vz256_cpu_tests tests/cpu_tests.cpp src/redcode_cpu.cpp)
        target_link_libraries(vz256_cpu_tests PRIVATE vz256_core Z80)
        target_compile_options(vz256_cpu_tests PRIVATE $<$<CXX_COMPILER_ID:GNU,Clang>:-UNDEBUG>)
        add_test(NAME cpu_bus COMMAND vz256_cpu_tests)
        add_executable(vz256_boot_tests tests/boot_tests.cpp src/redcode_cpu.cpp)
        target_link_libraries(vz256_boot_tests PRIVATE vz256_core Z80)
        target_compile_options(vz256_boot_tests PRIVATE $<$<CXX_COMPILER_ID:GNU,Clang>:-UNDEBUG>)
        add_test(NAME boot_disk COMMAND vz256_boot_tests "${CMAKE_SOURCE_DIR}")
    endif()
endif()
