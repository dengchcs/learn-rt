//
// Created by CD on 2022/10/26.
//

#ifndef RT_UTILS_H
#define RT_UTILS_H

#include "common.h"
#include <fstream>
#include <random>
#include <chrono>
#include <ctime>
#include <iomanip>

inline float clamp(float x, float xmin, float xmax) {
    return std::max(xmin, std::min(x, xmax));
}

float random_float(float lower = 0, float upper = 1) {
    std::uniform_real_distribution<float> dist(lower, upper);
    static std::random_device rd;
    static std::default_random_engine re(rd());
    return dist(re);
}

/**
 * @brief 生成[a, b]闭区间内的随机整数
 */
int random_int(int lower, int upper) {
    std::uniform_int_distribution<int> dist(lower, upper);
    static std::random_device rd;
    static std::default_random_engine re(rd());
    return dist(re);
}

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
point_t random_in_unit_sphere() {
    while (true) {
        const point_t point = random_in_cube(-1, 1);
        if (point.lengthSquared() < 1) {
            return point;
        }
    }
}

/**
 * @brief 在以normal为轴向的半球内随机采样一个方向
 */
vec3_t random_in_hemisphere(const vec3_t &normal) {
    vec3_t vec = random_in_unit_sphere();
    if (QVector3D::dotProduct(vec, normal) > 0) {
        return vec;
    } else {
        return -vec;
    }
}

/**
 * @brief 在位于XY平面、圆心在原点、半径为1的圆内部随机采样一点
 */
point_t random_in_unit_disk() {
    while (true) {
        const point_t point = {random_float(-1, 1), random_float(-1, 1), 0};
        if (point.lengthSquared() < 1) {
            return point;
        }
    }
}

/**
 * @brief 获取一个随机的单位向量
 */
unit_vec3 random_unit_vec() {
    return unit_vec3{random_in_unit_sphere()};
}

/**
 * @brief 计算反射光线的方向
 */
vec3_t reflect(const vec3_t &vec, const unit_vec3 &normal) {
    return vec - 2 * QVector3D::dotProduct(vec, normal) * normal;
}

/**
 * @brief 计算折射光线的方向
 * @param eta_ratio 入射材料折射率除以折射材料折射率
 * @return 在没有折射发生时返回std::nullopt
 */
std::optional<unit_vec3> refract(const unit_vec3 &vec, const unit_vec3 &normal, float eta_ratio) {
    const float cos_theta = std::min(QVector3D::dotProduct(-vec, normal), 1.0f);
    const float sin_theta = std::sqrt(1.0f - cos_theta * cos_theta);
    if (sin_theta * eta_ratio > 1.0f) {
        return std::nullopt;
    };
    const vec3_t out_perp = eta_ratio * (vec + cos_theta * normal);
    const vec3_t out_para = -std::sqrt(std::abs(1.0f - out_perp.lengthSquared())) * normal;
    return unit_vec3{out_perp + out_para};
}

void write_color(std::ofstream &file, color_t pixel, int samples_per_pixel = 1) {
    auto r = pixel.x(), g = pixel.y(), b = pixel.z();

    const float scale = 1.0f / samples_per_pixel;
    r = std::sqrt(r * scale);   // Gamma Correction (gamma = 0.5), gamma越小图片越亮
    g = std::sqrt(g * scale);
    b = std::sqrt(b * scale);

    file << static_cast<int>(256 * clamp(r, 0, 0.999)) << ' '
         << static_cast<int>(256 * clamp(g, 0, 0.999)) << ' '
         << static_cast<int>(256 * clamp(b, 0, 0.999)) << '\n';
}

std::string current_time() {
    std::time_t t = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    auto ltime = std::localtime(&t);
    std::stringstream ss;
    ss << std::put_time(ltime, "%H-%M-%S");
    return ss.str();
}

#endif //RT_UTILS_H
