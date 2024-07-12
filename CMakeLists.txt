# Minimum required version of CMake
cmake_minimum_required(VERSION 3.16.3)
set(CMAKE_CXX_STANDARD 17)

# Project name
project(VoronoiVisualization)

# # Find CGAL
# find_package(CGAL REQUIRED)

# Create the executable for box boundary
add_executable(VoronoiVisualization src/main.cpp)

# Link CGAL to the executable
target_link_libraries(VoronoiVisualization glfw)