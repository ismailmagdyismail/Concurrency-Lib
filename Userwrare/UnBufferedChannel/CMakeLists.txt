cmake_minimum_required(VERSION 3.17)

project(UNBUFFERED_CHANNELS_USERWARE LANGUAGES CXX)

# Specify C++17 standard and require it
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(ConcurrencyLib ../Concurrency-Lib)

add_executable(UNBUFFERED_CHANNELS_USERWARE main.cpp)

target_include_directories(UnBufferedChannelUserware PRIVATE
    ${ConcurrencyLib}/UnBufferedChannel
)

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_options(UNBUFFERED_CHANNELS_USERWARE PRIVATE -g)
endif()
