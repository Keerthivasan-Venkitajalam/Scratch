cmake_minimum_required(VERSION 3.15)
project(orderbook VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE Debug CACHE STRING "Build type" FORCE)
endif()

option(ORDERBOOK_ASAN "Enable AddressSanitizer" OFF)

if(ORDERBOOK_ASAN)
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
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

# Enable testing
enable_testing()

# Price Level Tests
add_executable(price_level_tests
    tests/price_level_tests.cpp
    src/orderbook/price_level.cpp
)

target_include_directories(price_level_tests PRIVATE ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(price_level_tests GTest::gtest_main)
target_compile_options(price_level_tests PRIVATE -Wall -Wextra -Werror)

# Order Book Tests
add_executable(order_book_tests
    tests/order_book_tests.cpp
    src/orderbook/order_book.cpp
    src/orderbook/price_level.cpp
)

target_include_directories(order_book_tests PRIVATE ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(order_book_tests GTest::gtest_main)
target_compile_options(order_book_tests PRIVATE -Wall -Wextra -Werror)

# Market Depth Test
add_executable(test_market_depth
    src/test_market_depth.cpp
    src/orderbook/order_book.cpp
    src/orderbook/price_level.cpp
)

target_include_directories(test_market_depth PRIVATE ${CMAKE_SOURCE_DIR}/include)
target_compile_options(test_market_depth PRIVATE -Wall -Wextra -Werror)

include(GoogleTest)
gtest_discover_tests(price_level_tests)
gtest_discover_tests(order_book_tests)
