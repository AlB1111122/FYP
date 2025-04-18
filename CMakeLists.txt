#cmake for linux ver
cmake_minimum_required(VERSION 3.10)

if(DEFINED ENV{PROJ_NAME})
  set(PROJECT_NAME $ENV{PROJ_NAME})
else()
  set(PROJECT_NAME "FYPVideoFilter")
endif()

file(GLOB PLATFORM_SRC src/platform/linux/*.cc)
file(GLOB COMMON_SRC src/common/*.cc)

project(${PROJECT_NAME} LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(SOURCES
  ${PLATFORM_SRC}
  ${COMMON_SRC}
)

find_package(SDL2 REQUIRED)
include_directories(SDL2Test ${SDL2_INCLUDE_DIRS})

include_directories(
  ${SDL2_INCLUDE_DIRS}
  include/common
)

add_compile_options(
  -O3
  -ftree-vectorize 
  -march=armv8-a+simd 
  -mcpu=cortex-a72 
  -mtune=cortex-a72 
  -falign-functions=32 
  -falign-loops=32
)

add_executable(${PROJECT_NAME} ${SOURCES})

target_link_libraries(${PROJECT_NAME} ${SDL2_LIBRARIES})