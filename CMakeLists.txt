# Minimum required version of CMake
cmake_minimum_required(VERSION 3.16.3)
set(CMAKE_CXX_STANDARD 17)

# Project name
project(LargestEmptyCircleVisual)

# Find CGAL
find_package(CGAL REQUIRED)

# Find OpenGL
find_package(OpenGL REQUIRED)

# Create the executable for LargestEmptyCircleVisual
add_executable(LargestEmptyCircleVisual
    src/largestEmptyCircleVisual.cpp
    src/glad.h
    src/glad.c
)

# Create the executable for LargestEmptyCircleDemo
add_executable(LargestEmptyCircleDemo
    src/largestEmptyCircleDemo.cpp
    src/glad.h
    src/glad.c
)

# Link CGAL to the executable
target_link_libraries(LargestEmptyCircleVisual
    glfw
    OpenGL::GL
    CGAL::CGAL
)

# Link CGAL to the executable
target_link_libraries(LargestEmptyCircleDemo
    glfw
    OpenGL::GL
    CGAL::CGAL
)

# Include the header files
target_include_directories(LargestEmptyCircleDemo PUBLIC include)

# Include the header files
target_include_directories(LargestEmptyCircleVisual PUBLIC include)