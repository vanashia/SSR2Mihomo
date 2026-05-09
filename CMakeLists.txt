cmake_minimum_required(VERSION 3.16)
project(ssr_decode LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(CURL REQUIRED)

add_library(ssr2mihomo
    src/encoding.cpp
    src/subscription.cpp
    src/ssr.cpp
    src/vmess.cpp
    src/mihomo.cpp
    src/fetcher.cpp
)

target_include_directories(ssr2mihomo PUBLIC include)
target_link_libraries(ssr2mihomo PUBLIC CURL::libcurl)

add_executable(SSR2Mihomo src/main.cpp)
target_link_libraries(SSR2Mihomo PRIVATE ssr2mihomo)

enable_testing()

add_executable(ssr2mihomo_tests
    tests/test_ssr2mihomo.cpp
)
target_link_libraries(ssr2mihomo_tests PRIVATE ssr2mihomo)

add_test(NAME ssr2mihomo_tests COMMAND ssr2mihomo_tests)
