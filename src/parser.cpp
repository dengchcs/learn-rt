#include "parser.hpp"

#include "happly.h"
#include "rectangle.hpp"
#include "sphere.hpp"
#include "tracer.hpp"
#include "triangle.hpp"

world_t parser::make_scene() const {
    std::cout << "scene reading: started...\n";
    const auto config = toml::parse_file(scene_path_.string());

    std::cout << "\treading textures...\n";
    const auto &textures = *config.get_as<toml::array>("textures");
    tex_tbl_t tex_tbl;
    for (auto &&tex : textures) {
        const auto &tex_info = *tex.as_array();
        const auto tex_name = tex_info[0].value<std::string>().value();
        const auto tex_type = tex_info[1].value<std::string>().value();
        if (tex_type == "image") {
            const auto path = tex_info[2].value<std::string>().value();
            auto image_path = scene_path_.parent_path();
            image_path /= path;
            exist_or_abort(image_path, "image file");
            tex_tbl.emplace(tex_name, std::make_shared<image_texture>(image_path.string()));
        } else if (tex_type == "solid") {
            const auto rgb = parse_vec(tex_info, 2, 3);
            tex_tbl.emplace(tex_name, std::make_shared<solid_color>(rgb[0], rgb[1], rgb[2]));
        } else {
            std::cerr << "unknown texture: " << tex_type << "\n";
        }
    }

    std::cout << "\treading materials...\n";
    const auto &materials = *config.get_as<toml::array>("materials");
    mat_tbl_t mat_tbl;
    for (auto &&mat : materials) {
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
                mat_tbl.emplace(mat_name,
                                std::make_shared<lambertian>(vec3_t{rgb[0], rgb[1], rgb[2]}));
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
                mat_tbl.emplace(
                    mat_name, std::make_shared<metal>(vec3_t{rgbf[0], rgbf[1], rgbf[2]}, rgbf[3]));
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
        for (auto &&sph : spheres) {
            const auto &sph_info = *sph.as_array();
            const auto xyzr = parse_vec(sph_info, 0, 4);
            const auto mat_name = sph_info[4].value<std::string>().value();
            const auto sph_mat = mat_tbl.at(mat_name);
            world.push_back(
                std::make_shared<sphere>(vec3_t{xyzr[0], xyzr[1], xyzr[2]}, xyzr[3], sph_mat));
        }
    }
    std::cout << "\treading triangles...\n";
    if (config.contains("triangles")) {
        const auto &triangles = *config.get_as<toml::array>("triangles");
        for (auto &&tri : triangles) {
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
            world.push_back(std::make_shared<triangle>(vertices[0], vertices[1], vertices[2],
                                                       tex_coords, tri_mat));
        }
    }
    std::cout << "\treading meshes...\n";
    if (config.contains("meshes")) {
        const auto &meshes = *config.get_as<toml::array>("meshes");
        float tex[6] = {0, 0, 0, 0, 0, 0};
        for (auto &&mesh : meshes) {
            const auto mesh_info = *mesh.as_array();
            auto mesh_name = mesh_info[0].value<std::string>().value();
            auto mesh_path = scene_path_.parent_path();
            mesh_path /= mesh_name;
            exist_or_abort(mesh_path, "mesh file");
            const auto mat_name = mesh_info[1].value<std::string>().value();
            const auto mesh_mat = mat_tbl.at(mat_name);

            const auto translate = parse_vec(*mesh_info[2].as_array(), 0, 3);
            const auto scale = mesh_info[3].value<double>().value();
            const auto rotate = parse_vec(*mesh_info[4].as_array(), 0, 3);
            transf transform{(float)scale, vec3_t{translate.data()}, vec3_t{rotate.data()}};
            for (int i = 0; i < 3; i++) {
                transform.rotate[i] *= (g_pi / 180);
            }

            happly::PLYData mesh_data(mesh_path.string());
            auto vertices = mesh_data.getVertexPositions();
            transform_mesh(vertices, transform);

            const auto indices = mesh_data.getFaceIndices();
            for (auto &&index : indices) {
                const auto v0 = vertices[index[0]], v1 = vertices[index[1]],
                           v2 = vertices[index[2]];
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
        for (auto &&rect : rectangles) {
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

tracer parser::make_tracer() const {
    std::cout << "tracer configuring: started...\n";
    const auto config = toml::parse_file(scene_path_.string());
    const int width = config["canvas"]["width"].value<int>().value();
    const int height = config["canvas"]["height"].value<int>().value();
    const auto bkg_vec = parse_vec(*config["canvas"]["background"].as_array(), 0, 3);
    const color_t bkg{bkg_vec.data()};
    const int samples_per_pixel = config["options"]["samples_per_pixel"].value<int>().value();
    const int max_depth = config["options"]["max_depth"].value<int>().value();
    const bool use_bvh = config["options"]["use_bvh"].value<bool>().value();
    const bool parallel = config["options"]["parallel"].value<bool>().value();
    tracer::config tconfig{width, height, samples_per_pixel, max_depth, use_bvh, parallel, bkg};

    const float aspect_ratio = (float)width / (float)height;

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

std::vector<float> parser::parse_vec(const toml::array &arr, int start, int cnt) {
    std::vector<float> vec(cnt);
    for (int i = start; i < start + cnt; i++) {
        vec[i - start] = arr[i].value<float>().value();
    }
    return vec;
}

void parser::transform_mesh(std::vector<std::array<double, 3>> &vertices, transf &trans) {
    using vert_t = std::array<double, 3>;

    auto rotate = [](const vert_t &vert, const vec3_t &rot) {
        const auto cx = cos(rot[0]), cy = cos(rot[1]), cz = cos(rot[2]);
        const auto sx = sin(rot[0]), sy = sin(rot[1]), sz = sin(rot[2]);
        auto res = vert;
        std::array<vert_t, 3> rot_mat = {
            vert_t{cy * cz, sx * sy * cz - cx * sz, cx * sy * cz + sx * sz},
            {cx * sz, sx * sy * sz + cx * cz, cx * sy * sz - sx * cz},
            {-sy, sx * cy, cx * cy}};
        for (int i = 0; i < 3; i++) {
            res[i] = std::inner_product(vert.begin(), vert.end(), rot_mat[i].begin(), 0.0);
        }
        return res;
    };

    const auto num_vert = vertices.size();
    auto point_add = [](const vert_t &vert1, const vert_t &vert2) {
        return vert_t{vert1[0] + vert2[0], vert1[1] + vert2[1], vert1[2] + vert2[2]};
    };
    auto center = std::reduce(std::execution::par, vertices.begin(), vertices.end(),
                              vert_t{0.0, 0.0, 0.0}, point_add);
    center[0] /= num_vert, center[1] /= num_vert, center[2] /= num_vert;
    auto distance = [](const vert_t &v1, const vert_t &v2) {
        const auto d0 = v1[0] - v2[0], d1 = v1[1] - v2[1], d2 = v1[2] - v2[2];
        return std::sqrt(d0 * d0 + d1 * d1 + d2 * d2);
    };
    const auto farmost = std::max_element(std::execution::par, vertices.begin(), vertices.end(),
                                          [&](const vert_t &v1, const vert_t v2) {
                                              return distance(v1, center) < distance(v2, center);
                                          });
    const auto dist = distance(*farmost, center);
    trans.scale /= dist;
    std::for_each(std::execution::par, vertices.begin(), vertices.end(), [&](auto &&vert) {
        vert[0] = (vert[0] - center[0]) * trans.scale;
        vert[1] = (vert[1] - center[1]) * trans.scale;
        vert[2] = (vert[2] - center[2]) * trans.scale;
        vert = rotate(vert, trans.rotate);
        vert[0] += trans.translate[0];
        vert[1] += trans.translate[1];
        vert[2] += trans.translate[2];
    });
}