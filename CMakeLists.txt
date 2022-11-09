cmake_minimum_required(VERSION 3.22)
project(rt)

set(CMAKE_CXX_STANDARD 17)

add_executable(rt main.cpp
        aabb.hpp bvh.hpp common.hpp camera.hpp
        hittable.hpp material.hpp parser.hpp ray.hpp tracer.hpp sphere.hpp utils.hpp
        texture.hpp triangle.hpp rectangle.hpp
        deps/stb_image_write.h deps/stb_image.h deps/toml.hpp)

if (UNIX)
    target_link_libraries(${PROJECT_NAME} tbb)
endif()
