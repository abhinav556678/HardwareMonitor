cmake_minimum_required(VERSION 3.16)
project(HardwareMonitor LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Include headers from the include/ directory
include_directories(${CMAKE_SOURCE_DIR}/include)

# Collect all source files
file(GLOB SOURCES "src/*.cpp")

add_executable(HardwareMonitor ${SOURCES})

# Enable warnings for cleaner code
if(MSVC)
    target_compile_options(HardwareMonitor PRIVATE /W4)
else()
    target_compile_options(HardwareMonitor PRIVATE -Wall -Wextra)
endif()
