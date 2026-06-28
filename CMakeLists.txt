cmake_minimum_required(VERSION 3.16)
project(franklin CXX)

# ---------------------------------------------------------------------------
# Build options (mirrors the USE_* flags at the top of the original Makefile)
# ---------------------------------------------------------------------------
option(USE_DEBUG     "Build with debug info, no optimization" OFF)
option(USE_64BIT      "Build 64-bit target (franklin64.exe)"   OFF)
option(USE_UNICODE    "Define UNICODE / _UNICODE"              ON)
option(USE_STATIC     "Statically link the runtime (-static)"  ON)
option(USE_VERBOSE    "Print full compiler/linker command lines during build" ON)

# When USE_VERBOSE is ON, every compile/link step prints its complete,
# unabbreviated command line (equivalent to the un-@-prefixed recipe lines
# in the original Makefile, or running `cmake --build build --verbose`
# without having to remember to type --verbose every time).
if(USE_VERBOSE)
    set(CMAKE_VERBOSE_MAKEFILE ON)
endif()

# USE_CLANG in the Makefile only selected which toolchain dir to look in
# (d:\clang\bin vs d:\tdm32\bin / d:\tdm64\bin). With CMake, select the
# compiler via -DCMAKE_CXX_COMPILER=clang++ (or g++) on the configure line,
# e.g.:
#   cmake -B build -DCMAKE_CXX_COMPILER=d:/clang/bin/clang++.exe
# CMAKE_CXX_COMPILER_ID is checked below so -DUSING_CLANG still gets added
# automatically when clang is in use, same as the Makefile did.

# ---------------------------------------------------------------------------
# Sources (CPPSRC from the Makefile)
# ---------------------------------------------------------------------------
set(FRANKLIN_SOURCES
    franklin.cpp
    file_handler.cpp
    der_libs/conio_min.cpp
    der_libs/common_funcs.cpp
    der_libs/qualify.cpp
)

if(USE_64BIT)
    set(BIN_NAME franklin64)
else()
    set(BIN_NAME franklin)
endif()

add_executable(${BIN_NAME} ${FRANKLIN_SOURCES})

# ---------------------------------------------------------------------------
# Include directories (-Ider_libs)
# ---------------------------------------------------------------------------
target_include_directories(${BIN_NAME} PRIVATE der_libs)

# ---------------------------------------------------------------------------
# Compile flags (CFLAGS)
# ---------------------------------------------------------------------------
# -Wall -O3 / -g, -Weffc++, -Wno-write-strings
target_compile_options(${BIN_NAME} PRIVATE
    -Wall
    -Weffc++
    -Wno-write-strings
)

if(USE_DEBUG)
    target_compile_options(${BIN_NAME} PRIVATE -g)
    set(CMAKE_BUILD_TYPE Debug)
else()
    target_compile_options(${BIN_NAME} PRIVATE -O3)
    set(CMAKE_BUILD_TYPE Release)
endif()

if(USE_64BIT)
    target_compile_definitions(${BIN_NAME} PRIVATE USE_64BIT)
endif()

if(USE_UNICODE)
    target_compile_definitions(${BIN_NAME} PRIVATE UNICODE _UNICODE)
endif()

if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    target_compile_definitions(${BIN_NAME} PRIVATE USING_CLANG)
endif()

# NOMAKEDEPEND was only needed by some `makedepend` versions; CMake tracks
# dependencies itself, so it's intentionally omitted here.

# ---------------------------------------------------------------------------
# Link flags (LFLAGS) and libraries (LIBS)
# ---------------------------------------------------------------------------
# -s -O3 (release) handled via CMAKE_BUILD_TYPE/strip below; -g (debug) via
# target_compile_options above already covers the compile half, this covers
# the link half:
if(NOT USE_DEBUG)
    target_link_options(${BIN_NAME} PRIVATE -s)
endif()

if(USE_STATIC)
    target_link_options(${BIN_NAME} PRIVATE -static)
endif()

target_link_libraries(${BIN_NAME} PRIVATE shlwapi comdlg32)

# ---------------------------------------------------------------------------
# Optional: clang-tidy integration (replaces the `check` target)
# Enable with: cmake -B build -DENABLE_CLANG_TIDY=ON
# ---------------------------------------------------------------------------
option(ENABLE_CLANG_TIDY "Run clang-tidy on build" OFF)
if(ENABLE_CLANG_TIDY)
    set(CLANG_TIDY_ARGS "-header-filter=.*" "--")
    list(APPEND CLANG_TIDY_ARGS "-Ider_libs")
    if(USE_64BIT)
        list(APPEND CLANG_TIDY_ARGS "-DUSE_64BIT")
    endif()
    if(USE_UNICODE)
        list(APPEND CLANG_TIDY_ARGS "-DUNICODE" "-D_UNICODE")
    endif()
    set(CMAKE_CXX_CLANG_TIDY clang-tidy ${CLANG_TIDY_ARGS})
endif()

# ---------------------------------------------------------------------------
# distclean target ("make clean" equivalent)
# ---------------------------------------------------------------------------
# Note: a custom target cannot delete the build directory it is currently
# running from, so instead this wipes out everything CMake generated inside
# it (cache, generated makefiles, object files, the .exe) while leaving the
# empty build/ folder itself in place. Run it from the SOURCE directory:
#
#   cmake --build build --target distclean
#
# After running it, "build" is back to an empty shell and you can
# reconfigure with the same cmake -B build ... command you used originally.
add_custom_target(distclean
    COMMAND ${CMAKE_COMMAND} -E remove -f CMakeCache.txt
    COMMAND ${CMAKE_COMMAND} -E remove -f cmake_install.cmake
    COMMAND ${CMAKE_COMMAND} -E remove -f Makefile
    COMMAND ${CMAKE_COMMAND} -E rm -rf CMakeFiles
    COMMAND ${CMAKE_COMMAND} -E rm -rf "${CMAKE_BINARY_DIR}/CMakeFiles"
    COMMAND ${CMAKE_COMMAND} -E remove -f "$<TARGET_FILE:${BIN_NAME}>"
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    COMMENT "Removing all CMake-generated and build files (distclean)"
    VERBATIM
)
