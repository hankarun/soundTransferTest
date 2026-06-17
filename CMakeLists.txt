cmake_minimum_required(VERSION 3.16)
project(soundTransfer LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

# ---------------------------------------------------------------------------
# Qt5 (assumed installed system-wide; set Qt5_DIR or CMAKE_PREFIX_PATH if not
# found automatically, e.g. -DCMAKE_PREFIX_PATH=C:/Qt/5.15.2/msvc2019_64)
# ---------------------------------------------------------------------------
find_package(Qt5 REQUIRED COMPONENTS Core Network Multimedia Widgets Test)

# ---------------------------------------------------------------------------
# libopus via FetchContent
# ---------------------------------------------------------------------------
include(FetchContent)

# Disable opus's own extras so only the library target is built.
set(OPUS_BUILD_TESTING OFF CACHE BOOL "" FORCE)
set(OPUS_BUILD_SHARED_LIBRARY OFF CACHE BOOL "" FORCE)
set(OPUS_BUILD_PROGRAMS OFF CACHE BOOL "" FORCE)

# Opus's AVX2 kernels use intrinsics (e.g. _mm_loadu_si64) that the older GCC
# in the MinGW 8.1 toolchain shipped with Qt 5.15 lacks. Disable the AVX2
# codepath there (SSE/SSE2/SSE4.1 runtime detection still applies). MSVC and
# modern GCC/Clang compile the AVX2 path fine, so leave it enabled for them.
if(MINGW)
    set(OPUS_X86_MAY_HAVE_AVX2 OFF CACHE BOOL "" FORCE)
    set(OPUS_X86_PRESUME_AVX2 OFF CACHE BOOL "" FORCE)
endif()

FetchContent_Declare(opus
    GIT_REPOSITORY https://github.com/xiph/opus.git
    GIT_TAG        v1.5.2
)
FetchContent_MakeAvailable(opus)
# opus exports the target `opus` (alias `Opus::opus`).

enable_testing()

add_subdirectory(src/core)
add_subdirectory(src/app)
add_subdirectory(tests)
