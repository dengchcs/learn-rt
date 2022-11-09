//
// Created by CD on 2022/10/31.
//

#ifndef RT_PARSER_HPP
#define RT_PARSER_HPP

#include "camera.hpp"
#include "hittable.hpp"
#include "material.hpp"
#include "rectangle.hpp"
#include "sphere.hpp"
#include "tracer.hpp"
#include "triangle.hpp"
#include "deps/happly.h"
#include "deps/toml.hpp"
#include <algorithm>
#include <execution>
#include <map>
#include <numeric>
#include <string>
#include <utility>

class parser {
    std::string file_;

    static std::vector<float> parse_vec(const toml::array &arr, int start, int cnt) {
        std::vector<float> vec(cnt);
        for (int i = start; i < start + cnt; i++) {
            vec[i - start] = arr[i].value<float>().value();
        }
        return vec;
    }

    static void transform_mesh(std::vector<std::array<double, 3>> &vertices, const vec3_t &translate, double scale) {
        using vert_t = std::array<double, 3>;
        const auto num_vert = vertices.size();
        auto point_add = [](const vert_t &vert1, const vert_t &vert2) {
            return vert_t{vert1[0] + vert2[0], vert1[1] + vert2[1], vert1[2] + vert2[2]};
        };
        auto center = std::reduce(std::execution::par_unseq, vertices.begin(), vertices.end(), vert_t{0.0, 0.0, 0.0},
                                  point_add);
        center[0] /= num_vert, center[1] /= num_vert, center[2] /= num_vert;
        auto distance = [](const vert_t &v1, const vert_t &v2) {
            const auto d0 = v1[0] - v2[0], d1 = v1[1] - v2[1], d2 = v1[2] - v2[2];
            return std::sqrt(d0 * d0 + d1 * d1 + d2 * d2);
        };
        const auto farmost = std::max_element(std::execution::par_unseq,
                                              vertices.begin(), vertices.end(), [&](const vert_t &v1, const vert_t v2) {
                    return distance(v1, center) < distance(v2, center);
                });
        const auto dist = distance(*farmost, center);
        scale /= dist;
        std::for_each(std::execution::par_unseq, vertices.begin(), vertices.end(), [&](auto &&vert) {
            vert[0] = (vert[0] - center[0]) * scale + translate[0];
            vert[1] = (vert[1] - center[1]) * scale + translate[1];
            vert[2] = (vert[2] - center[2]) * scale + translate[2];
        });
    }

public:
    explicit parser(std::string file) : file_(std::move(file)) {}

    [[nodiscard]] world_t make_scene() const {
        std::cout << "scene reading: started...\n";
        const auto config = toml::parse_file(file_);

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
            } else if (mat_type == "light") {
                if (use_tex) {
                    const auto tex_name = mat_info[3].value<std::string>().value();
                    mat_tbl.emplace(mat_name, std::make_shared<light>(tex_tbl.at(tex_name)));
                } else {
                    const auto rgb = parse_vec(mat_info, 3, 3);
                    mat_tbl.emplace(mat_name, std::make_shared<light>(color_t{rgb[0], rgb[1], rgb[2]}));
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
                const auto trans = parse_vec(*mesh_info[2].as_array(), 0, 3);
                const vec3_t translate{trans.data()};
                const auto scale = mesh_info[3].value<double>().value();
                happly::PLYData mesh_data(mesh_name);
                auto vertices = mesh_data.getVertexPositions();
                transform_mesh(vertices, translate, scale);
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
        std::cout << "\treading rectangles...\n";
        if (config.contains("rectangles")) {
            const auto &rectangles = *config.get_as<toml::array>("rectangles");
            for (auto &&rect: rectangles) {
                const auto rect_info = *rect.as_array();
                point_t points[3];
                for (int i = 0; i < 3; i++) {
                    const auto vert = parse_vec(*rect_info[i].as_array(), 0, 3);
                    points[i] = point_t{vert.data()};
                }
                const auto mat_name = rect_info[3].value<std::string>().value();
                const auto rect_mat = mat_tbl.at(mat_name);
                world.push_back(std::make_shared<rectangle>(points[0], points[1], points[2], rect_mat));
            }
        }

        std::cout << "scene reading: done.\n\n";
        return world;
    }

    [[nodiscard]] tracer make_tracer() const {
        std::cout << "tracer configuring: started...\n";
        const auto config = toml::parse_file(file_);
        const int width = config["canvas"]["width"].value<int>().value();
        const int height = config["canvas"]["height"].value<int>().value();
        const auto bkg_vec = parse_vec(*config["canvas"]["background"].as_array(), 0, 3);
        const color_t bkg{bkg_vec.data()};
        const int samples_per_pixel = config["options"]["samples_per_pixel"].value<int>().value();
        const int max_depth = config["options"]["max_depth"].value<int>().value();
        const bool use_bvh = config["options"]["use_bvh"].value<bool>().value();
        const bool parallel = config["options"]["parallel"].value<bool>().value();
        tracer::config tconfig{
                width, height,
                samples_per_pixel,
                max_depth, use_bvh, parallel,
                bkg
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
};

#endif //RT_PARSER_HPP
