cmake_minimum_required(VERSION 3.20)
project(wolf3d_port C)

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

add_executable(wolf3d_port
  src/main.c
  src/platform/platform_stub.c
)

target_include_directories(wolf3d_port PRIVATE include)
