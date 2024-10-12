#cmake for linux ver
cmake_minimum_required(VERSION 3.10)

if(DEFINED ENV{PROJ_NAME})
    set(PROJECT_NAME $ENV{PROJ_NAME})
else()
   set(PROJECT_NAME "FYPVideoFilter")
endif()

project(${PROJECT_NAME} LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(SOURCES
    src/display_sdl.cpp
    src/filter.cpp
)

find_package(SDL2 REQUIRED)
include_directories(SDL2Test ${SDL2_INCLUDE_DIRS})

include_directories(${SDL2_INCLUDE_DIRS})

add_executable(${PROJECT_NAME} ${SOURCES})

target_link_libraries(${PROJECT_NAME} ${SDL2_LIBRARIES})