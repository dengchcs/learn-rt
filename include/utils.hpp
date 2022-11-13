#ifndef RT_UTILS_HPP
#define RT_UTILS_HPP

#include <filesystem>
#include <iostream>
#include <optional>

#include "common.hpp"

inline float clamp(float x, float xmin, float xmax) { return std::max(xmin, std::min(x, xmax)); }

float random_float(float lower = 0, float upper = 1);

/**
 * @brief 生成[a, b]闭区间内的随机整数
 */
int random_int(int lower, int upper);

/**
 * @brief 在[xmin, xmax)^3的立方体内部随机采样一点(包含表面)
 */
inline point_t random_in_cube(float xmin, float xmax) {
    return {random_float(xmin, xmax), random_float(xmin, xmax), random_float(xmin, xmax)};
}

/**
 * @brief 在球心在原点、半径为1的球内部(不包含球表面)随机采样一点
 * @return
 */
point_t random_in_unit_sphere();

/**
 * @brief 在以normal为轴向的半球内随机采样一个方向
 */
vec3_t random_in_hemisphere(const vec3_t &normal);

/**
 * @brief 在位于XY平面、圆心在原点、半径为1的圆内部随机采样一点
 */
point_t random_in_unit_disk();

/**
 * @brief 获取一个随机的单位向量
 */
inline unit_vec3 random_unit_vec() { return unit_vec3{random_in_unit_sphere()}; }

/**
 * @brief 计算反射光线的方向
 */
inline vec3_t reflect(const vec3_t &vec, const unit_vec3 &normal) {
    return vec - 2 * vec.dot(normal) * normal;
}

/**
 * @brief 计算折射光线的方向
 * @param eta_ratio 入射材料折射率除以折射材料折射率
 * @return 在没有折射发生时返回std::nullopt
 */
std::optional<unit_vec3> refract(const unit_vec3 &vec, const unit_vec3 &normal, float eta_ratio);

/**
 * @brief 获取%H-%M-%S表示的时间串字符
 */
std::string current_time();

/**
 * @brief 检查文件(夹)是否存在, 如不存在则终止程序
 * @param path 文件路径
 * @param err_leader 错误信息头
 */
inline void exist_or_abort(const std::filesystem::path &path, const std::string &err_leader) {
    if (!std::filesystem::exists(path)) {
        std::cerr << err_leader << ' ' << path << " does not exist!\n";
        std::abort();
    }
}

#endif  // RT_UTILS_HPP
