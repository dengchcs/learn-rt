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

inline point_t random_in_cube(float xmin, float xmax) {
    return {random_float(xmin, xmax), random_float(xmin, xmax), random_float(xmin, xmax)};
}

point_t random_in_unit_sphere() {
    while (true) {
        const point_t point = random_in_cube(-1, 1);
        if (point.lengthSquared() < 1) {
            return point;
        }
    }
}

vec3_t random_in_unit_disk() {
    while (true) {
        const vec3_t vec = {random_float(-1, 1), random_float(-1, 1), 0};
        if (vec.lengthSquared() >= 1) continue;
        return vec;
    }
}

unit_vec3 random_unit_vec() {
    return unit_vec3{random_in_unit_sphere()};
}

vec3_t reflect(const vec3_t &vec, const unit_vec3& normal) {
    return vec - 2 * QVector3D::dotProduct(vec, normal) * normal;
}

/**
 * @param eta_ratio 入射材料折射率除以折射材料折射率
 */
std::optional<unit_vec3> refract(const unit_vec3& vec, const unit_vec3& normal, float eta_ratio) {
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
    r = std::sqrt(r * scale);
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
