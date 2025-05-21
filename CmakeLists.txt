cmake_minimum_required(VERSION 3.26)
project(hello C CXX)

find_package(SDL3 REQUIRED)

add_executable(hello)

target_sources(hello
PRIVATE
    main.cpp
)

target_link_libraries(hello SDL3::SDL3)