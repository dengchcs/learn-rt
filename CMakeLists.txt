cmake_minimum_required(VERSION 3.22)
project(rt)

set(CMAKE_CXX_STANDARD 17)

add_executable(rt main.cpp
        aabb.h bvh_node.h common.h camera.h hittable.h material.h parser.h ray.h tracer.h sphere.h utils.h
        deps/stb_image_write.h deps/toml.hpp)