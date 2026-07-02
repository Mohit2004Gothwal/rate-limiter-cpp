cmake_minimum_required(VERSION 3.14)
project(RateLimiter VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ── Compiler warnings ─────────────────────────────────────────────────────────
if(MSVC)
    add_compile_options(/W4)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# ── Include directories ───────────────────────────────────────────────────────
include_directories(include)

# ── Shared library of all algorithm implementations ───────────────────────────
set(LIB_SOURCES
    src/TokenBucket.cpp
    src/FixedWindowCounter.cpp
    src/SlidingWindowLog.cpp
    src/SlidingWindowCounter.cpp
    src/LeakyBucket.cpp
    src/RateLimiterMiddleware.cpp
)

add_library(rate_limiter_lib STATIC ${LIB_SOURCES})
target_include_directories(rate_limiter_lib PUBLIC include)

# Link threads (needed for std::mutex on Linux)
find_package(Threads REQUIRED)
target_link_libraries(rate_limiter_lib Threads::Threads)

# ── Main demo executable ──────────────────────────────────────────────────────
add_executable(rate_limiter_demo src/main.cpp)
target_link_libraries(rate_limiter_demo rate_limiter_lib)

# ── Test executable ───────────────────────────────────────────────────────────
add_executable(rate_limiter_tests tests/test_all.cpp)
target_link_libraries(rate_limiter_tests rate_limiter_lib)

# ── CTest integration ─────────────────────────────────────────────────────────
enable_testing()
add_test(NAME UnitTests COMMAND rate_limiter_tests)
