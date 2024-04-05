cmake_minimum_required(VERSION 3.0)
project(MyRenderer)

# Set the C++ compiler
set(CMAKE_CXX_COMPILER g++)

# Set the C++ flags
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")

# Include directories
include_directories(${PROJECT_SOURCE_DIR})

# Find all the source files
file(GLOB SOURCES "*.cpp")

# Define the target executable and its sources
add_executable(${PROJECT_NAME} ${SOURCES})

# Link with math library
target_link_libraries(${PROJECT_NAME} m)
