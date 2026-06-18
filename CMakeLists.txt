cmake_minimum_required(VERSION 3.16)
project(OS_third_project)

set(CMAKE_CXX_STANDARD 20)

# Define the source files explicitly
set(SOURCES
        src/VirtualMemory.cpp
        src/PhysicalMemory.cpp
        tests/test0_sanity.cpp
)

# Create the executable from the source files
add_executable(OS_third_project ${SOURCES})

# Tell CMake where to look for header (.h) files
target_include_directories(OS_third_project PRIVATE src tests)