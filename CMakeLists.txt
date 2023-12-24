cmake_minimum_required(VERSION 3.12)

project(PIKProject)

# Set C++ version
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Find necessary packages
find_package(glfw3 REQUIRED)
find_package(glm REQUIRED)
find_package(GLUT REQUIRED)
find_package(GLEW REQUIRED)



# Set source files
set(SOURCE_FILES
        src/main.cpp
        src/physics/physic_object.hpp
        src/physics/physics.hpp
        src/physics/collision_grip.hpp
        src/physics/grid.hpp
        imgui/imgui.cpp
        imgui/imgui_draw.cpp
        imgui/imconfig.h
        imgui/imgui.h
        imgui/imgui_tables.cpp
        imgui/imgui_internal.h
        imgui/imgui_widgets.cpp
        imgui/imgui_demo.cpp
        imgui/backends/imgui_impl_glut.h
        imgui/backends/imgui_impl_glut.cpp
        imgui/backends/imgui_impl_opengl2.h
        imgui/backends/imgui_impl_opengl2.cpp
)


# Include directories
include_directories(
        ${OPENGL_INCLUDE_DIRS}
        ${GLUT_INCLUDE_DIRS}
        imgui
)

# Create executable
add_executable(${PROJECT_NAME} ${SOURCE_FILES})

# Link libraries
target_link_libraries(${PROJECT_NAME}
        ${OPENGL_LIBRARIES}
        ${GLUT_LIBRARY}
        GL GLU glut
        ${CMAKE_THREAD_LIBS_INIT}
)

# Set compiler flags for optimization and warnings
if (CMAKE_COMPILER_IS_GNUCXX)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -pedantic")
endif()

# Set output directory
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)
set(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)