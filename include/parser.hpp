#ifndef RT_PARSER_HPP
#define RT_PARSER_HPP

#include <filesystem>
#include <functional>
#include <memory>
#include <unordered_map>

#include "hittable.hpp"
#include "toml.hpp"
#include "utils.hpp"

class material;
class texture;
class tracer;

class parser {
    std::filesystem::path scene_path_;  // 配置文件的(绝对)路径

    // 用于对mesh作几何变换
    struct transf {
        double scale{1};   // 网格点距离模型"中心"的距离
        vec3_t translate;  // 模型"中心"的位置
        vec3_t rotate;     // 模型绕XYZ轴的旋转角度(弧度制)
    };

    /**
     * @brief 从toml数组解析出一个浮点数组
     *
     * @param arr toml数组
     * @param start 起始下标, 从0开始
     * @param cnt 期望的数组长度
     */
    static std::vector<float> parse_vec(const toml::array &arr, int start, int cnt);

    /**
     * @brief 对一个网格模型作几何变换
     *
     * @param vertices 网格点
     * @param trans 几何变换定义
     */
    static void transform_mesh(std::vector<std::array<double, 3>> &vertices, transf &trans);

    using tex_tbl_t = std::unordered_map<std::string, std::shared_ptr<texture>>;
    [[nodiscard]] tex_tbl_t read_textures() const;

    using mat_tbl_t = std::unordered_map<std::string, std::shared_ptr<material>>;
    [[nodiscard]] mat_tbl_t read_materials(const tex_tbl_t &tex_tbl) const;

    /**
     * @brief 单个物体(如球/矩形/mesh)是由一个toml数组定义的, 同一类物体位于一个toml数组内.
     * 对此类物体作解析
     *
     * @param name 物体类别
     * @param single_parser 用于解析单个物体的函数
     */
    void read_objects(const char *name,
                      const std::function<void(const toml::array &info)> &single_parser) const;

public:
    explicit parser(const std::string &file) : scene_path_(file) {
        scene_path_ = std::filesystem::absolute(scene_path_);
        exist_or_abort(scene_path_, "scene config file");
    }

    [[nodiscard]] world_t make_scene() const;

    [[nodiscard]] tracer make_tracer() const;
};

#endif  // RT_PARSER_HPP
