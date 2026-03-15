cmake_minimum_required(VERSION 3.15)
project(feedhandler VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE Debug CACHE STRING "Build type" FORCE)
endif()

option(FEED_ASAN "Enable AddressSanitizer" OFF)

if(FEED_ASAN)
	message(STATUS "AddressSanitizer enabled")
	add_compile_options(-fsanitize=address -fno-omit-frame-pointer)
	add_link_options(-fsanitize=address)
endif()

# Fetch Google Test
include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
)
# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

# Fetch Google Benchmark
FetchContent_Declare(
  googlebenchmark
  URL https://github.com/google/benchmark/archive/refs/tags/v1.8.3.zip
)
set(BENCHMARK_ENABLE_TESTING OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googlebenchmark)

# Enable testing
enable_testing()

add_executable(feedhandler 
    src/main.cpp
    src/net/tcp_client.cpp
    src/net/event_loop.cpp
    src/net/receive_buffer.cpp
    src/net/websocket_client.cpp
)

# Add parser benchmark executable
add_executable(parser_benchmark
    src/parser_benchmark.cpp
    src/parser/naive_fix_parser.cpp
    src/parser/stringview_fix_parser.cpp
    src/parser/optimized_fix_parser.cpp
)

# Add fast number parser test executable
add_executable(test_fast_number_parser
    src/test_fast_number_parser.cpp
)

# Add repeating group parser test executable
add_executable(test_repeating_groups
    src/test_repeating_groups.cpp
    src/parser/repeating_group_parser.cpp
)

# Add FSM parser test executable
add_executable(test_fsm_parser
    src/test_fsm_parser.cpp
    src/parser/fsm_fix_parser.cpp
)

# Add streaming handler test executable (FSM + Buffer integration)
add_executable(test_streaming_handler
    src/test_streaming_handler.cpp
    src/parser/streaming_fix_handler.cpp
    src/parser/fsm_fix_parser.cpp
    src/net/receive_buffer.cpp
)

# Add branch prediction test executable
add_executable(test_branch_prediction
    src/test_branch_prediction.cpp
    src/parser/fsm_fix_parser.cpp
)

# Add tick pool test executable
add_executable(test_tick_pool
    src/test_tick_pool.cpp
    src/common/tick_pool.cpp
)

# Add garbage recovery test executable
add_executable(test_garbage_recovery
    src/test_garbage_recovery.cpp
    src/parser/fsm_fix_parser.cpp
)

# Add threaded feedhandler test executable
add_executable(test_threaded_feedhandler
    src/test_threaded_feedhandler.cpp
    src/threading/threaded_feedhandler.cpp
    src/parser/fsm_fix_parser.cpp
)

# Add final integration demo
add_executable(final_demo
    src/final_demo.cpp
    src/parser/fsm_fix_parser.cpp
    src/threading/threaded_feedhandler.cpp
)

target_include_directories(feedhandler PRIVATE ${CMAKE_SOURCE_DIR}/include)
target_include_directories(parser_benchmark PRIVATE ${CMAKE_SOURCE_DIR}/include)
target_include_directories(test_fast_number_parser PRIVATE ${CMAKE_SOURCE_DIR}/include)
target_include_directories(test_repeating_groups PRIVATE ${CMAKE_SOURCE_DIR}/include)
target_include_directories(test_fsm_parser PRIVATE ${CMAKE_SOURCE_DIR}/include)
target_include_directories(test_streaming_handler PRIVATE ${CMAKE_SOURCE_DIR}/include)
target_include_directories(test_branch_prediction PRIVATE ${CMAKE_SOURCE_DIR}/include)
target_include_directories(test_tick_pool PRIVATE ${CMAKE_SOURCE_DIR}/include)
target_include_directories(test_garbage_recovery PRIVATE ${CMAKE_SOURCE_DIR}/include)
target_include_directories(test_threaded_feedhandler PRIVATE ${CMAKE_SOURCE_DIR}/include)
target_include_directories(final_demo PRIVATE ${CMAKE_SOURCE_DIR}/include)

target_compile_options(feedhandler PRIVATE -Wall -Wextra -Werror)
target_compile_options(parser_benchmark PRIVATE -Wall -Wextra -Werror)
target_compile_options(test_fast_number_parser PRIVATE -Wall -Wextra -Werror)
target_compile_options(test_repeating_groups PRIVATE -Wall -Wextra -Werror)
target_compile_options(test_fsm_parser PRIVATE -Wall -Wextra -Werror)
target_compile_options(test_streaming_handler PRIVATE -Wall -Wextra -Werror)
target_compile_options(test_branch_prediction PRIVATE -Wall -Wextra -Werror)
target_compile_options(test_tick_pool PRIVATE -Wall -Wextra -Werror)
target_compile_options(test_garbage_recovery PRIVATE -Wall -Wextra -Werror)
target_compile_options(test_threaded_feedhandler PRIVATE -Wall -Wextra -Werror)
target_compile_options(final_demo PRIVATE -Wall -Wextra -Werror)

# Add mock FIX server
add_executable(mock_fix_server
    src/mock_fix_server.cpp
)

# Add final feedhandler demo
add_executable(feedhandler_demo
    src/feedhandler_demo.cpp
    src/parser/fsm_fix_parser.cpp
    src/net/receive_buffer.cpp
)

target_include_directories(feedhandler_demo PRIVATE ${CMAKE_SOURCE_DIR}/include)
target_compile_options(mock_fix_server PRIVATE -Wall -Wextra -Werror)
target_compile_options(feedhandler_demo PRIVATE -Wall -Wextra -Werror)


# Add GTest unit tests
add_executable(fsm_parser_tests
    tests/fsm_parser_tests.cpp
    src/parser/fsm_fix_parser.cpp
)

target_include_directories(fsm_parser_tests PRIVATE ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(fsm_parser_tests GTest::gtest_main)
target_compile_options(fsm_parser_tests PRIVATE -Wall -Wextra -Werror)

include(GoogleTest)
gtest_discover_tests(fsm_parser_tests)

# Add Google Benchmark suite
add_executable(gbench_parsers
    benchmarks/bench_parsers.cpp
    src/parser/naive_fix_parser.cpp
    src/parser/stringview_fix_parser.cpp
    src/parser/fsm_fix_parser.cpp
)

target_include_directories(gbench_parsers PRIVATE ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(gbench_parsers benchmark::benchmark)
target_compile_options(gbench_parsers PRIVATE -Wall -Wextra -Werror)

# Add ultimate performance test suite with all optimizations
add_executable(ultimate_performance_test
    benchmarks/ultimate_performance_test.cpp
    src/parser/simd_fix_parser.cpp
    src/common/numa_memory_pool.cpp
    src/threading/ultra_low_latency_queue.cpp
    src/benchmarks/hardware_profiler.cpp
    src/common/zero_latency_allocator.cpp
)

target_include_directories(ultimate_performance_test PRIVATE ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(ultimate_performance_test benchmark::benchmark)

# Platform-specific optimizations
if(CMAKE_SYSTEM_PROCESSOR MATCHES "arm64|aarch64")
    target_compile_options(ultimate_performance_test PRIVATE -Wall -Wextra -Werror -mcpu=native)
else()
    target_link_libraries(ultimate_performance_test numa)
    target_compile_options(ultimate_performance_test PRIVATE -Wall -Wextra -Werror -mavx2 -mfma)
endif()

# Add SIMD parser test
add_executable(test_simd_parser
    src/test_simd_parser.cpp
    src/parser/simd_fix_parser.cpp
)

target_include_directories(test_simd_parser PRIVATE ${CMAKE_SOURCE_DIR}/include)

# Platform-specific optimizations for SIMD test
if(CMAKE_SYSTEM_PROCESSOR MATCHES "arm64|aarch64")
    target_compile_options(test_simd_parser PRIVATE -Wall -Wextra -Werror -mcpu=native)
else()
    target_compile_options(test_simd_parser PRIVATE -Wall -Wextra -Werror -mavx2 -mfma)
endif()

# Add zero latency allocator test
add_executable(test_zero_latency_allocator
    src/test_zero_latency_allocator.cpp
    src/common/zero_latency_allocator.cpp
)

target_include_directories(test_zero_latency_allocator PRIVATE ${CMAKE_SOURCE_DIR}/include)
target_compile_options(test_zero_latency_allocator PRIVATE -Wall -Wextra -Werror)

# Add hardware profiler test
add_executable(test_hardware_profiler
    src/test_hardware_profiler.cpp
    src/benchmarks/hardware_profiler.cpp
)

target_include_directories(test_hardware_profiler PRIVATE ${CMAKE_SOURCE_DIR}/include)
target_compile_options(test_hardware_profiler PRIVATE -Wall -Wextra -Werror)
