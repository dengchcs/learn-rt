//
// Created by CD on 2022/10/31.
//

#ifndef RT_PARSER_HPP
#define RT_PARSER_HPP

#include "camera.hpp"
#include "hittable.hpp"
#include "material.hpp"
#include "sphere.hpp"
#include "tracer.hpp"
#include "triangle.hpp"
#include "deps/happly.h"
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
    std::cout << "scene reading: started...\n";
    const auto config = toml::parse_file(file);

    std::cout << "\treading textures...\n";
    const auto &textures = *config.get_as<toml::array>("textures");
    std::map<std::string, std::shared_ptr<texture>> tex_tbl;
    for (auto &&tex: textures) {
        const auto &tex_info = *tex.as_array();
        const auto tex_name = tex_info[0].value<std::string>().value();
        const auto tex_type = tex_info[1].value<std::string>().value();
        if (tex_type == "image") {
            const auto path = tex_info[2].value<std::string>().value();
            tex_tbl.emplace(tex_name, std::make_shared<image_texture>(path));
        } else if (tex_type == "solid") {
            const auto rgb = parse_vec(tex_info, 2, 3);
            tex_tbl.emplace(tex_name, std::make_shared<solid_color>(rgb[0], rgb[1], rgb[2]));
        } else {
            std::cerr << "unknown texture: " << tex_type << "\n";
        }
    }

    std::cout << "\treading materials...\n";
    const auto &materials = *config.get_as<toml::array>("materials");
    std::map<std::string, std::shared_ptr<material>> mat_tbl;
    for (auto &&mat: materials) {
        const auto mat_info = *mat.as_array();
        const auto mat_name = mat_info[0].value<std::string>().value();
        const auto mat_type = mat_info[1].value<std::string>().value();
        const auto use_tex = mat_info[2].value<bool>().value();
        if (mat_type == "lambertian") {
            if (use_tex) {
                const auto tex_name = mat_info[3].value<std::string>().value();
                mat_tbl.emplace(mat_name, std::make_shared<lambertian>(tex_tbl.at(tex_name)));
            } else {
                const auto rgb = parse_vec(mat_info, 3, 3);
                mat_tbl.emplace(mat_name, std::make_shared<lambertian>(vec3_t{rgb[0], rgb[1], rgb[2]}));
            }
        } else if (mat_type == "dielectric") {
            if (use_tex) {

            } else {
                const auto ref_idx = mat_info[3].value<float>().value();
                mat_tbl.emplace(mat_name, std::make_shared<dielectric>(ref_idx));
            }
        } else if (mat_type == "metal") {
            if (use_tex) {
                const auto tex_name = mat_info[3].value<std::string>().value();
                const auto fuzz = mat_info[4].value<float>().value();
                mat_tbl.emplace(mat_name, std::make_shared<metal>(tex_tbl.at(tex_name), fuzz));
            } else {
                const auto rgbf = parse_vec(mat_info, 3, 4);
                mat_tbl.emplace(mat_name, std::make_shared<metal>(vec3_t{rgbf[0], rgbf[1], rgbf[2]}, rgbf[3]));
            }
        } else {
            std::cerr << "unknown material: " << mat_type << "\n";
        }
    }

    world_t world;
    std::cout << "\treading spheres...\n";
    if (config.contains("spheres")) {
        const auto &spheres = *config.get_as<toml::array>("spheres");
        for (auto &&sph: spheres) {
            const auto &sph_info = *sph.as_array();
            const auto xyzr = parse_vec(sph_info, 0, 4);
            const auto mat_name = sph_info[4].value<std::string>().value();
            const auto sph_mat = mat_tbl.at(mat_name);
            world.push_back(std::make_shared<sphere>(vec3_t{xyzr[0], xyzr[1], xyzr[2]}, xyzr[3], sph_mat));
        }
    }
    std::cout << "\treading triangles...\n";
    if (config.contains("triangles")) {
        const auto &triangles = *config.get_as<toml::array>("triangles");
        for (auto &&tri: triangles) {
            const auto &tri_info = *tri.as_array();
            point_t vertices[3];
            float tex_coords[6];
            for (int i = 0; i < 3; i++) {
                const auto vert = parse_vec(*tri_info[i].as_array(), 0, 5);
                vertices[i] = {vert[0], vert[1], vert[2]};
                tex_coords[i * 2] = vert[3], tex_coords[i * 2 + 1] = vert[4];
            }
            const auto mat_name = tri_info[3].value<std::string>().value();
            const auto tri_mat = mat_tbl.at(mat_name);
            world.push_back(std::make_shared<triangle>(vertices[0], vertices[1], vertices[2], tex_coords, tri_mat));
        }
    }
    std::cout << "\treading meshes...\n";
    if (config.contains("meshes")) {
        const auto &meshes = *config.get_as<toml::array>("meshes");
        float tex[6] = {0, 0, 0, 0, 0, 0};
        for (auto &&mesh: meshes) {
            const auto mesh_info = *mesh.as_array();
            const auto mesh_name = mesh_info[0].value<std::string>().value();
            const auto mat_name = mesh_info[1].value<std::string>().value();
            const auto mesh_mat = mat_tbl.at(mat_name);
            happly::PLYData mesh_data(mesh_name);
            const auto vertices = mesh_data.getVertexPositions();
            const auto indices = mesh_data.getFaceIndices();
            for (auto &&index: indices) {
                const auto v0 = vertices[index[0]], v1 = vertices[index[1]], v2 = vertices[index[2]];
                const auto p0 = point_t(v0[0], v0[1], v0[2]);
                const auto p1 = point_t(v1[0], v1[1], v1[2]);
                const auto p2 = point_t(v2[0], v2[1], v2[2]);
                world.push_back(std::make_shared<triangle>(p0, p1, p2, tex, mesh_mat));
            }
        }
    }
    std::cout << "\treading lights...\n";
    if (config.contains("lights")) {
        const auto &lights = *config.get_as<toml::array>("lights");
        float tex[6] = {0, 0, 0, 0, 0, 0};
        for (auto &&ligh : lights) {
            const auto light_info = *ligh.as_array();
            point_t points[3];
            for (int i = 0; i < 3; i++) {
                const auto vert = parse_vec(*light_info[i].as_array(), 0, 3);
                points[i] = point_t{vert.data()};
            }
            const auto rgb = parse_vec(light_info, 3, 3);
            const auto mat = std::make_shared<light>(color_t{rgb[0], rgb[1], rgb[2]});
            world.push_back(std::make_shared<triangle>(points[0], points[1], points[2], tex, mat));
        }
    }

    std::cout << "scene reading: done.\n\n";
    return world;
}

tracer make_tracer(const std::string &file) {
    std::cout << "tracer configuring: started...\n";
    const auto config = toml::parse_file(file);
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

    auto parse_vec3 = [](const toml::array &arr) {
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

    std::cout << "tracer configuring: done.\n\n";
    return {tconfig, cam};
}

#endif //RT_PARSER_HPP
