cmake_minimum_required(VERSION 3.22)
project(aplay)

# Set C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_FLAGS "-Wall -Wextra -Wpedantic") # TODO: -Werror

# Find PortAudio
find_package(PkgConfig REQUIRED)
pkg_check_modules(PORTAUDIO REQUIRED IMPORTED_TARGET portaudio-2.0)

# Add raylib
include_directories(${PROJECT_SOURCE_DIR}/raylib/include)
link_directories(${PROJECT_SOURCE_DIR}/raylib/lib)

# Add your sources here
set(SOURCES src/main.cpp)

# Add executable
add_executable(${PROJECT_NAME} ${SOURCES})
add_executable(raysin src/raysin.cpp)

# Link libraries
target_link_libraries(${PROJECT_NAME} PkgConfig::PORTAUDIO)
target_link_libraries(raysin raylib)

# Set LD_LIBRARY_PATH
set_target_properties(${PROJECT_NAME} 
    PROPERTIES 
    BUILD_WITH_INSTALL_RPATH TRUE 
    INSTALL_RPATH "${PROJECT_SOURCE_DIR}/raylib/lib")