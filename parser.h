//
// Created by CD on 2022/10/31.
//

#ifndef RT_PARSER_H
#define RT_PARSER_H

#include "sphere.h"
#include "hittable.h"
#include "material.h"
#include "camera.h"
#include "tracer.h"

#include "deps/toml.hpp"
#include <map>
#include <string>

world_t random_world() {
    auto material_ground = std::make_shared<lambertian>(color_t{0.8, 0.8, 0.0});

    world_t world;
    world.push_back(std::make_shared<sphere>(point_t{0, -1000, 0}, 1000, material_ground));
    for (int a = -11; a < 11; a++) {
        for (int b = -11; b < 11; b++) {
            auto choose_mat = random_float();
            point_t center(a + 0.9f * random_float(), 0.2, b + 0.9f * random_float());

            if ((center - point_t(4, 0.2, 0)).len() > 0.9) {
                std::shared_ptr<material> sphere_material;

                if (choose_mat < 0.8) {
                    // diffuse
                    vec3_t albedo = {random_float(), random_float(), random_float()};
                    sphere_material = std::make_shared<lambertian>(albedo);
                    world.push_back(std::make_shared<sphere>(center, 0.2, sphere_material));
                } else if (choose_mat < 0.95) {
                    // metal
                    vec3_t albedo = {random_float(0.5, 1), random_float(0.5, 1), random_float(0.5, 1)};
                    auto fuzz = random_float(0, 0.5);
                    sphere_material = std::make_shared<metal>(albedo, fuzz);
                    world.push_back(std::make_shared<sphere>(center, 0.2, sphere_material));
                } else {
                    // glass
                    sphere_material = std::make_shared<dielectric>(1.5);
                    world.push_back(std::make_shared<sphere>(center, 0.2, sphere_material));
                }
            }
        }
    }

    auto material1 = std::make_shared<dielectric>(1.5);
    world.push_back(std::make_shared<sphere>(point_t(0, 1, 0), 1.0, material1));

    auto material2 = std::make_shared<lambertian>(vec3_t{0.4, 0.2, 0.1});
    world.push_back(std::make_shared<sphere>(point_t(-4, 1, 0), 1.0, material2));

    auto material3 = std::make_shared<metal>(vec3_t(0.7, 0.6, 0.5), 0.0);
    world.push_back(std::make_shared<sphere>(point_t(4, 1, 0), 1.0, material3));

    return world;
}

std::vector<float> parse_vec(const toml::array &arr, int start, int cnt) {
    std::vector<float> vec(cnt);
    for (int i = start; i < start + cnt; i++) {
        vec[i - start] = arr[i].value<float>().value();
    }
    return vec;
}

world_t make_scene(const std::string &file) {
    auto config = toml::parse_file(file);
    auto& materials = *config.get_as<toml::array>("materials");
    std::map<std::string, std::shared_ptr<material>> mat_tbl;
    for (auto &&mat: materials) {
        auto mat_arr = *mat.as_array();
        auto mat_name = mat_arr[0].value<std::string>().value();
        auto mat_type = mat_arr[1].value<std::string>().value();
        if (mat_type == "lambertian") {
            const auto rgb = parse_vec(mat_arr, 2, 3);
            mat_tbl.emplace(mat_name, std::make_shared<lambertian>(vec3_t{rgb[0], rgb[1], rgb[2]}));
        } else if (mat_type == "dielectric") {
            auto ref_idx = mat_arr[2].value<float>().value();
            mat_tbl.emplace(mat_name, std::make_shared<dielectric>(ref_idx));
        } else if (mat_type == "metal") {
            const auto rgbf = parse_vec(mat_arr, 2, 4);
            mat_tbl.emplace(mat_name, std::make_shared<metal>(vec3_t{rgbf[0], rgbf[1], rgbf[2]}, rgbf[3]));
        }
    }

    world_t world;
    auto& spheres = *config.get_as<toml::array>("spheres");
    for (auto &&sph: spheres) {
        auto sph_arr = *sph.as_array();
        const auto xyzr = parse_vec(sph_arr, 0, 4);
        auto mat_name = sph_arr[4].value<std::string>().value();
        auto sph_mat = mat_tbl[mat_name];
        world.push_back(std::make_shared<sphere>(vec3_t{xyzr[0], xyzr[1], xyzr[2]}, xyzr[3], sph_mat));
    }

    return world;
}

tracer make_tracer(const std::string &file) {
    auto config = toml::parse_file(file);
    const int width = config["canvas"]["width"].value<int>().value();
    const int height = config["canvas"]["height"].value<int>().value();
    const int samples_per_pixel = config["options"]["samples_per_pixel"].value<int>().value();
    const int max_depth = config["options"]["max_depth"].value<int>().value();
    const bool use_bvh = config["options"]["use_bvh"].value<bool>().value();
    const bool parallel = config["options"]["parallel"].value<bool>().value();
    tracer::config tconfig{
            width, height,
            samples_per_pixel,
            max_depth, use_bvh, parallel,
    };

    const float aspect_ratio = (float) width / (float) height;

    auto parse_vec3 = [](const toml::array& arr) {
        const auto vec = parse_vec(arr, 0, 3);
        return vec3_t{vec[0], vec[1], vec[2]};
    };
    const auto lookfrom = parse_vec3(*config["camera"]["lookfrom"].as_array());
    const auto lookat = parse_vec3(*config["camera"]["lookat"].as_array());
    const auto vup = parse_vec3(*config["camera"]["vup"].as_array());
    const float dist_to_focus = (lookfrom - lookat).len();
    const float aperture = config["camera"]["aperture"].value<float>().value();
    const float vfov = config["camera"]["vfov"].value<float>().value();
    camera cam{lookfrom, lookat, vup, vfov, aspect_ratio, aperture, dist_to_focus};

    return {tconfig, cam};
}



#endif //RT_PARSER_H
