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

target_include_directories(feedhandler PRIVATE ${CMAKE_SOURCE_DIR}/include)
target_include_directories(parser_benchmark PRIVATE ${CMAKE_SOURCE_DIR}/include)
target_include_directories(test_fast_number_parser PRIVATE ${CMAKE_SOURCE_DIR}/include)

target_compile_options(feedhandler PRIVATE -Wall -Wextra -Werror)
target_compile_options(parser_benchmark PRIVATE -Wall -Wextra -Werror)
target_compile_options(test_fast_number_parser PRIVATE -Wall -Wextra -Werror)

