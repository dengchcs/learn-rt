#include "parser.hpp"

#include <array>

#include "tiny_obj_loader.h"
#include "rectangle.hpp"
#include "sphere.hpp"
#include "tracer.hpp"
#include "triangle.hpp"

std::vector<float> parser::parse_vec(const toml::array &arr, int start, int cnt) {
    std::vector<float> vec(cnt);
    for (int i = start; i < start + cnt; i++) {
        vec[i - start] = arr[i].value<float>().value();
    }
    return vec;
}

void parser::transform_mesh(std::vector<point_t> &vertices, transf &trans) {
    const auto rot = trans.rotate;
    const auto cosx = std::cos(rot[0]);
    const auto cosy = std::cos(rot[1]);
    const auto cosz = std::cos(rot[2]);
    const auto sinx = std::sin(rot[0]);
    const auto siny = std::sin(rot[1]);
    const auto sinz = std::sin(rot[2]);
    const std::array<vec3_t, 3> rot_mat = {
        vec3_t{cosy * cosz, sinx * siny * cosz - cosx * sinz, cosx * siny * cosz + sinx * sinz},
        {cosx * sinz, sinx * siny * sinz + cosx * cosz, cosx * siny * sinz - sinx * cosz},
        {-siny, sinx * cosy, cosx * cosy}
    };
    auto rotate = [&rot_mat](const point_t &vert) {
        auto res = vert;
        for (int i = 0; i < 3; i++) {
            res[i] = rot_mat[i][0] * vert[0] + rot_mat[i][1] * vert[1] + rot_mat[i][2] * vert[2];
            // res[i] = std::inner_product(vert.begin(), vert.end(), rot_mat.at(i).begin(), 0.0);
        }
        return res;
    };

    auto center = std::reduce(std::execution::par, vertices.begin(), vertices.end(),
                              point_t{0.0, 0.0, 0.0}, std::plus<point_t>());
    const auto num_vert = (int)vertices.size();
    center[0] /= num_vert, center[1] /= num_vert, center[2] /= num_vert;
    auto distance = [](const point_t &vert1, const point_t &vert2) {
        const auto diff = vert1 - vert2;
        return diff.len();
    };
    const auto farmost =
        std::max_element(std::execution::par, vertices.begin(), vertices.end(),
                         [&](const point_t &vert1, const point_t vert2) {
                             return distance(vert1, center) < distance(vert2, center);
                         });
    const auto dist = distance(*farmost, center);
    trans.scale /= dist;
    std::for_each(std::execution::par, vertices.begin(), vertices.end(), [&](auto &&vert) {
        vert = (vert - center) * trans.scale;
        vert = rotate(vert);
        vert += trans.translate;
    });
}

auto parser::read_textures() const -> tex_tbl_t {
    const auto config = toml::parse_file(scene_path_.string());
    std::cout << "\treading textures...\n";

    const auto &textures = *config.get_as<toml::array>("textures");
    tex_tbl_t tex_tbl;
    tex_tbl.reserve(textures.size());
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
            std::cerr << "unknown texture: " << tex_type << '\n';
        }
    }
    return tex_tbl;
}

auto parser::read_materials(const tex_tbl_t &tex_tbl) const -> mat_tbl_t {
    const auto config = toml::parse_file(scene_path_.string());
    std::cout << "\treading materials...\n";

    const auto &materials = *config.get_as<toml::array>("materials");
    mat_tbl_t mat_tbl;
    mat_tbl.reserve(materials.size());
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
        } else if (mat_type == "pbr") {
            const int info_len = mat_info.size();
            float metalic = mat_info[info_len - 2].value<float>().value();
            float roughness = mat_info[info_len - 1].value<float>().value();
            auto metalic_tex = std::make_shared<solid_color>(metalic, 0.0, 0.0);
            auto roughness_tex = std::make_shared<solid_color>(roughness, 0.0, 0.0);
            std::shared_ptr<texture> diffuse;
            if (use_tex) {
                const auto tex_name = mat_info[3].value<std::string>().value();
                diffuse = tex_tbl.at(tex_name);
            } else {
                const auto rgb = parse_vec(mat_info, 3, 3);
                diffuse = std::make_shared<solid_color>(rgb[0], rgb[1], rgb[2]);
            }
            mat_tbl.emplace(mat_name, std::make_shared<pbr>(diffuse, metalic_tex, roughness_tex));
        } else {
            std::cerr << "unknown material: " << mat_type << "\n";
        }
    }
    return mat_tbl;
}

void parser::read_objects(const char *name,
                          const std::function<void(const toml::array &info)> &single_parser) const {
    const auto config = toml::parse_file(scene_path_.string());
    if (!config.contains(name)) {
        return;
    }
    const auto &objects = *config.get_as<toml::array>(name);
    for (auto &&object : objects) {
        const auto info = *object.as_array();
        single_parser(info);
    }
}

world_t parser::make_scene() const {
    std::cout << "scene reading: started...\n";

    std::cout << "\treading textures...\n";
    const auto tex_tbl = read_textures();

    std::cout << "\treading materials...\n";
    const auto mat_tbl = read_materials(tex_tbl);

    world_t world;

    std::cout << "\treading spheres...\n";
    read_objects("spheres", [&](const toml::array &info) {
        const auto xyzr = parse_vec(info, 0, 4);
        const auto mat_name = info[4].value<std::string>().value();
        const auto &sph_mat = mat_tbl.at(mat_name);
        world.push_back(
            std::make_shared<sphere>(vec3_t{xyzr[0], xyzr[1], xyzr[2]}, xyzr[3], sph_mat));
    });

    std::cout << "\treading rectangles...\n";
    read_objects("rectangles", [&](const toml::array &rect_info) {
        std::array<point_t, 3> points;
        for (int i = 0; i < 3; i++) {
            const auto vert = parse_vec(*rect_info[i].as_array(), 0, 3);
            points.at(i) = point_t{vert.data()};
        }
        const auto mat_name = rect_info[3].value<std::string>().value();
        const auto &rect_mat = mat_tbl.at(mat_name);
        world.push_back(std::make_shared<rectangle>(points[0], points[1], points[2], rect_mat));
    });

    std::cout << "\treading triangles...\n";
    read_objects("triangles", [&](const toml::array &tri_info) {
        std::array<point_t, 3> vertices;
        std::array<tex_coords_t, 3> tex_coords;
        for (std::size_t i = 0; i < 3; i++) {
            const auto vert = parse_vec(*tri_info[i].as_array(), 0, 5);
            vertices.at(i) = {vert[0], vert[1], vert[2]};
            tex_coords.at(i).at(0) = vert[3], tex_coords.at(i).at(1) = vert[4];
        }
        const auto mat_name = tri_info[3].value<std::string>().value();
        const auto &tri_mat = mat_tbl.at(mat_name);
        world.push_back(
            std::make_shared<triangle>(vertices[0], vertices[1], vertices[2], tex_coords[0], tex_coords[1], tex_coords[2], tri_mat));
    });

    std::cout << "\treading meshes...\n";
    read_objects("meshes", [&](const toml::array &mesh_info) {
        auto mesh_name = mesh_info[0].value<std::string>().value();
        auto mesh_path = scene_path_.parent_path();
        mesh_path /= mesh_name;
        exist_or_abort(mesh_path, "mesh file");

        const auto translate = parse_vec(*mesh_info[1].as_array(), 0, 3);
        const auto scale = mesh_info[2].value<double>().value();
        const auto rotate = parse_vec(*mesh_info[3].as_array(), 0, 3);
        transf transform{(float)scale, vec3_t{translate.data()}, vec3_t{rotate.data()}};
        for (int i = 0; i < 3; i++) {
            transform.rotate[i] *= (g_pi / g_pi_in_degree);
        }

        tinyobj::ObjReader reader;
        tinyobj::ObjReaderConfig reader_config;
        reader_config.triangulate = true;
        if (! reader.ParseFromFile(mesh_path.string(), reader_config)) {
            if (!reader.Error().empty()) {
                std::cerr << "TinyObjReader: " << reader.Error();
            }
            exit(1);
        }
        if (!reader.Warning().empty()) {
            std::cout << "TinyObjReader: " << reader.Warning();
        }
        const auto& attrib = reader.GetAttrib();
        const auto& shapes = reader.GetShapes();
        const auto& materials = reader.GetMaterials();

        std::vector<std::shared_ptr<material>> mesh_materials;
        const auto use_tbl_mat = mesh_info[4].value<bool>().value();
        if (use_tbl_mat) {
            const auto mat_name = mesh_info[5].value<std::string>().value();
            mesh_materials.push_back(mat_tbl.at(mat_name));
        } else {
            for (auto&& mat : materials) {
                texture* diffuse_ptr;
                if (mat.diffuse_texname.size() == 0) {
                    std::cout << "\tno diffuse texture, solid color only\n";
                    const auto color = color_t{mat.diffuse};
                    diffuse_ptr = new solid_color{color};
                } else {
                    std::cout << "\tdiffuse texture found\n";
                    const auto texpath = mesh_path.parent_path() / mat.diffuse_texname;
                    diffuse_ptr = new image_texture{texpath.string()};
                }
                auto diffuse = std::shared_ptr<texture>{diffuse_ptr};
                
                texture* metalic_ptr;
                if (mat.metallic_texname.size() == 0) {
                    std::cout << "\tno metallic texture, solid color only\n";
                    const auto color = color_t{mat.metallic, 0.0, 0.0};
                    metalic_ptr = new solid_color{color};
                } else {
                    std::cout << "\tmetallic texture found\n";
                    const auto texpath = mesh_path.parent_path() / mat.metallic_texname;
                    metalic_ptr = new image_texture{texpath.string()};
                }
                auto metalic = std::shared_ptr<texture>{metalic_ptr};

                texture* roughness_ptr;
                if (mat.roughness_texname.size() == 0) {
                    std::cout << "\tno roughness texture, solid color only\n";
                    const auto color = color_t{mat.roughness, 0.0, 0.0};
                    roughness_ptr = new solid_color{color};
                } else {
                    std::cout << "\troughness texture found\n";
                    const auto texpath = mesh_path.parent_path() / mat.roughness_texname;
                    roughness_ptr = new image_texture{texpath.string()};
                }
                auto roughness = std::shared_ptr<texture>{roughness_ptr};

                mesh_materials.push_back(std::make_shared<pbr>(diffuse, metalic, roughness));
            }
        }

        // TODO: directly transform attrib.vertices and create triangles while looping
        std::vector<point_t> vertices;
        std::vector<tex_coords_t> tex_coords;
        using face_t = std::array<int, 3>;
        std::vector<face_t> faces;
        std::vector<int> mat_idx;
        // Loop over shapes
        for (size_t s = 0; s < shapes.size(); s++) {
            // Loop over faces(polygon)
            size_t index_offset = 0;
            for (size_t f = 0; f < shapes[s].mesh.num_face_vertices.size(); f++) {
                size_t fv = size_t(shapes[s].mesh.num_face_vertices[f]);
                assert(fv == 3); // only triangles are supported

                // Loop over vertices in the face.
                for (size_t v = 0; v < fv; v++) {
                    // access to vertex
                    tinyobj::index_t idx = shapes[s].mesh.indices[index_offset + v];
                    tinyobj::real_t vx = attrib.vertices[3 * size_t(idx.vertex_index) + 0];
                    tinyobj::real_t vy = attrib.vertices[3 * size_t(idx.vertex_index) + 1];
                    tinyobj::real_t vz = attrib.vertices[3 * size_t(idx.vertex_index) + 2];
                    vertices.emplace_back(vx, vy, vz);

                    // Check if `texcoord_index` is zero or positive. negative = no texcoord data
                    if (idx.texcoord_index >= 0) {
                        tinyobj::real_t tx = attrib.texcoords[2 * size_t(idx.texcoord_index) + 0];
                        tinyobj::real_t ty = attrib.texcoords[2 * size_t(idx.texcoord_index) + 1];
                        tex_coords.push_back(tex_coords_t{tx, ty});
                    } else {
                        tex_coords.push_back(tex_coords_t{0.0, 0.0});
                    }
                }
                index_offset += fv;
                const int nvert = (int)vertices.size();
                faces.push_back(face_t{nvert - 3, nvert - 2, nvert - 1});
                // per-face material
                if (use_tbl_mat) {
                    mat_idx.push_back(0);
                } else {
                    mat_idx.push_back(shapes[s].mesh.material_ids[f]);
                }
            }
        }
        transform_mesh(vertices, transform);
        const int nface = (int)faces.size();
        for (int i = 0; i < nface; i++) {
            const auto [v0, v1, v2] = faces[i];
            const int matidx = mat_idx[i];  // TODO: can be -1 when no material is assigned
            world.push_back(std::make_shared<triangle>(
                vertices[v0], vertices[v1], vertices[v2],
                tex_coords[v0], tex_coords[v1], tex_coords[v2], mesh_materials[matidx])
            );
        }
    });

    std::cout << "scene reading: done.\n\n";

    return world;
}

tracer parser::make_tracer() const {
    std::cout << "tracer configuring: started...\n";
    const auto config = toml::parse_file(scene_path_.string());
    const int width = config["canvas"]["width"].value<int>().value();
    const int height = config["canvas"]["height"].value<int>().value();
    const int samples_per_pixel = config["options"]["samples_per_pixel"].value<int>().value();
    const int max_depth = config["options"]["max_depth"].value<int>().value();
    const bool use_bvh = config["options"]["use_bvh"].value<bool>().value();
    const bool parallel = config["options"]["parallel"].value<bool>().value();
    tracer::config tconfig{width, height, samples_per_pixel, max_depth, use_bvh, parallel};

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

    const auto envinfo = *config["envlight"].as_array();
    texture* envlight_ptr;
    if (envinfo[0].value<bool>().value()) {
        const auto envlight_path = envinfo[1].value<std::string>().value();
        envlight_ptr = new image_texture{envlight_path};
    } else {
        const auto rgb = parse_vec(envinfo, 1, 3);
        envlight_ptr = new solid_color{rgb[0], rgb[1], rgb[2]};
    }
    auto envlight = std::unique_ptr<texture>{envlight_ptr};
    std::cout << "tracer configuring: done.\n\n";
    return {tconfig, cam, std::move(envlight)};
}
