#include "utils.hpp"

#include <ctime>
#include <iomanip>
#include <optional>
#include <random>
#include <sstream>

float random_float(float lower, float upper) {
    std::uniform_real_distribution<float> dist(lower, upper);
    static std::random_device rd;
    static std::default_random_engine re(rd());
    return dist(re);
}

int random_int(int lower, int upper) {
    std::uniform_int_distribution<int> dist(lower, upper);
    static std::random_device rd;
    static std::default_random_engine re(rd());
    return dist(re);
}

point_t random_in_unit_sphere() {
    while (true) {
        const point_t point = random_in_cube(-1, 1);
        if (point.len_sq() < 1) {
            return point;
        }
    }
}

vec3_t random_in_hemisphere(const vec3_t &normal) {
    vec3_t vec = random_in_unit_sphere();
    if (vec.dot(normal) > 0) {
        return vec;
    }
    return -vec;
}

point_t random_in_unit_disk() {
    while (true) {
        const point_t point = {random_float(-1, 1), random_float(-1, 1), 0};
        if (point.len_sq() < 1) {
            return point;
        }
    }
}

std::optional<unit_vec3> refract(const unit_vec3 &vec, const unit_vec3 &normal, float eta_ratio) {
    const float cos_theta = std::min(-vec.dot(normal), 1.0F);
    const float sin_theta = std::sqrt(1.0F - cos_theta * cos_theta);
    if (sin_theta * eta_ratio > 1.0F) {
        return std::nullopt;
    };
    const vec3_t out_perp = eta_ratio * (vec + cos_theta * normal);
    const vec3_t out_para = -std::sqrt(std::abs(1.0F - out_perp.len_sq())) * normal;
    return unit_vec3{out_perp + out_para};
}

std::string current_time() {
    std::time_t t = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    auto *ltime = std::localtime(&t);
    std::stringstream ss;
    ss << std::put_time(ltime, "%H-%M-%S");
    return ss.str();
}

float srgb_to_linear(float srgb) {
    return srgb <= 0.04045 ? srgb / 12.92 : std::pow((srgb + 0.055) / 1.055, 2.4);
}

float linear_to_srgb(float linear) {
    return linear <= 0.0031308 ? 12.92 * linear : 1.055 * std::pow(linear, 1.0 / 2.4) - 0.055;
}
