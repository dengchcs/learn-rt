#ifndef RT_PARSER_HPP
#define RT_PARSER_HPP

#include <filesystem>
#include <memory>
#include <unordered_map>

#include "../deps/toml.hpp"
#include "hittable.hpp"
#include "utils.hpp"

class material;
class texture;
class tracer;

class parser {
    std::filesystem::path scene_path_;  // 配置文件的(绝对)路径

    struct transf {
        double scale{1};
        vec3_t translate;
        vec3_t rotate;
    };

    static std::vector<float> parse_vec(const toml::array &arr, int start, int cnt);

    static void transform_mesh(std::vector<std::array<double, 3>> &vertices, transf &trans);

    using mat_tbl_t = std::unordered_map<std::string, std::shared_ptr<material>>;
    using tex_tbl_t = std::unordered_map<std::string, std::shared_ptr<texture>>;

    // mat_tbl_t mat_tbl_;
    // tex_tbl_t tex_tbl_;

    void parse_materials() const;

    void parse_textures() const;

public:
    explicit parser(const std::string &file) : scene_path_(file) {
        scene_path_ = std::filesystem::absolute(scene_path_);
        exist_or_abort(scene_path_, "scene config file");
    }

    [[nodiscard]] world_t make_scene() const;

    [[nodiscard]] tracer make_tracer() const;
};

#endif  // RT_PARSER_HPP
