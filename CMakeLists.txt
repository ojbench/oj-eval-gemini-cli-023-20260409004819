cmake_minimum_required(VERSION 3.10)
project(deque)

set(CMAKE_CXX_STANDARD 17)

file(GLOB SOURCES "*.cpp")
if(SOURCES)
    add_executable(code ${SOURCES})
endif()
