cmake_minimum_required(VERSION 3.16)
project(learn-rt LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# 默认使用 Release 模式; 多配置生成器 (如 Visual Studio) 需在构建时用 --config 指定
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

add_executable(rt
    src/bvh.cpp
    src/hittable.cpp
    src/main.cpp
    src/material.cpp
    src/parser.cpp
    src/rectangle.cpp
    src/sphere.cpp
    src/triangle.cpp
    src/utils.cpp
)

target_include_directories(rt PRIVATE include deps)
target_compile_definitions(rt PRIVATE TINYOBJLOADER_IMPLEMENTATION)

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(rt PRIVATE -Wall)
endif()

# std::execution::par 在 Linux 上依赖 tbb
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    target_link_libraries(rt PRIVATE tbb)
endif()
