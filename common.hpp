//
// Created by CD on 2022/10/25.
//

#ifndef RT_COMMON_HPP
#define RT_COMMON_HPP

#include <array>
#include <cassert>
#include <cmath>
#include <limits>

class unit_vec3;

class vector3 {
    std::array<float, 3> e;

public:
    vector3() : e{0, 0, 0} {}

    vector3(float x, float y, float z) : e{x, y, z} {}

    explicit vector3(const float *const data) : e{data[0], data[1], data[2]} {}

    [[nodiscard]] float x() const { return e[0]; }

    [[nodiscard]] float y() const { return e[1]; }

    [[nodiscard]] float z() const { return e[2]; }

    [[nodiscard]] float operator[](int i) const { return e[i]; }

    [[nodiscard]] float &operator[](int i) { return e[i]; }

    [[nodiscard]] float len() const { return std::sqrt(len_sq()); }

    [[nodiscard]] float len_sq() const {
        return e[0] * e[0] + e[1] * e[1] + e[2] * e[2];
    }

    void normalize() {
        const auto l = len();
        if (l == 0) return;
        for (float &i : e) {
            i /= l;
        }
    }

    [[nodiscard]] unit_vec3 normalized() const;

    [[nodiscard]] float dot(const vector3 &other) const {
        return e[0] * other.e[0] + e[1] * other.e[1] + e[2] * other.e[2];
    }

    [[nodiscard]] vector3 cross(const vector3 &other) const {
        return {e[1] * other.e[2] - e[2] * other.e[1],
                e[2] * other.e[0] - e[0] * other.e[2],
                e[0] * other.e[1] - e[1] * other.e[0]};
    }

    vector3 &operator+=(const vector3 &other) {
        e[0] += other.e[0];
        e[1] += other.e[1];
        e[2] += other.e[2];
        return *this;
    }

    vector3 &operator-=(const vector3 &other) {
        e[0] -= other.e[0];
        e[1] -= other.e[1];
        e[2] -= other.e[2];
        return *this;
    }

    vector3 &operator*=(float factor) {
        e[0] *= factor;
        e[1] *= factor;
        e[2] *= factor;
        return *this;
    }

    vector3 &operator/=(float factor) {
        assert(factor != 0);
        e[0] /= factor;
        e[1] /= factor;
        e[2] /= factor;
        return *this;
    }

    inline friend vector3 operator+(const vector3 &v1, const vector3 &v2) {
        return {v1.e[0] + v2.e[0], v1.e[1] + v2.e[1], v1.e[2] + v2.e[2]};
    }

    inline friend vector3 operator-(const vector3 &v1, const vector3 &v2) {
        return {v1.e[0] - v2.e[0], v1.e[1] - v2.e[1], v1.e[2] - v2.e[2]};
    }

    inline friend vector3 operator*(const vector3 &v1, const vector3 &v2) {
        return {v1.e[0] * v2.e[0], v1.e[1] * v2.e[1], v1.e[2] * v2.e[2]};
    }

    inline friend vector3 operator*(const vector3 &v1, float factor) {
        return {v1.e[0] * factor, v1.e[1] * factor, v1.e[2] * factor};
    }

    inline friend vector3 operator*(float factor, const vector3 &v1) {
        return {v1.e[0] * factor, v1.e[1] * factor, v1.e[2] * factor};
    }

    inline friend vector3 operator/(const vector3 &v1, float divisor) {
        assert(divisor != 0);
        return {v1.e[0] / divisor, v1.e[1] / divisor, v1.e[2] / divisor};
    }

    inline friend vector3 operator-(const vector3 &v1) {
        return {-v1.e[0], -v1.e[1], -v1.e[2]};
    }

    inline friend bool operator==(const vector3 &v1, const vector3 &v2) {
        return v1.e[0] == v2.e[0] && v1.e[1] == v2.e[1] && v1.e[2] == v2.e[2];
    }

    inline friend bool operator!=(const vector3 &v1, const vector3 &v2) {
        return v1.e[0] != v2.e[0] || v1.e[1] != v2.e[1] || v1.e[2] != v2.e[2];
    }
};

class unit_vec3 : public vector3 {
    unit_vec3(float x, float y, float z) : vector3(x, y, z) {}

public:
    unit_vec3() : vector3() {}

    explicit unit_vec3(const vector3 &vec) : vector3(vec) { this->normalize(); }

    inline friend unit_vec3 operator-(const unit_vec3 &v) {
        return {-v[0], -v[1], -v[2]};
    }
};

inline unit_vec3 vector3::normalized() const { return unit_vec3{*this}; }

using color_t = vector3;
using vec3_t = vector3;
using point_t = vector3;

constexpr auto g_infinity = std::numeric_limits<float>::max();
constexpr float g_pi = 3.141592653589793238462643383279502884;

#endif  // RT_COMMON_HPP
