cmake_minimum_required(VERSION 3.16)
project(FFForge CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ── Sources ────────────────────────────────────────────────────────────────────
add_executable(ffforge
    src/main.cpp
    src/encoder.cpp
    src/decoder.cpp
    src/transcoder.cpp
    src/utils.cpp
)

target_include_directories(ffforge PRIVATE include)

# ── FFmpeg ─────────────────────────────────────────────────────────────────────
# Requires libavcodec, libavformat, libavutil to be installed system-wide
# (e.g. `sudo apt install libavcodec-dev libavformat-dev libavutil-dev`).
find_package(PkgConfig REQUIRED)
pkg_check_modules(FFMPEG REQUIRED
    libavcodec
    libavformat
    libavutil
)

target_include_directories(ffforge PRIVATE ${FFMPEG_INCLUDE_DIRS})
target_link_libraries(ffforge PRIVATE ${FFMPEG_LIBRARIES})
target_compile_options(ffforge PRIVATE ${FFMPEG_CFLAGS_OTHER})

# ── Warnings ───────────────────────────────────────────────────────────────────
if(MSVC)
    target_compile_options(ffforge PRIVATE /W4)
else()
    target_compile_options(ffforge PRIVATE -Wall -Wextra -Wpedantic)
endif()
