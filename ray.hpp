//
// Created by CD on 2022/10/25.
//

#include "common.hpp"

#ifndef RT_RAY_H
#define RT_RAY_H

class ray {
private:
    point_t origin_;
    unit_vec3 direction_;

public:
    [[nodiscard]] point_t origin() const { return origin_; }

    [[nodiscard]] unit_vec3 direction() const { return direction_; }

public:
    ray() = default;
    ray(const point_t &origin, const vec3_t &direction)
        : origin_(origin), direction_(direction) {}

    [[nodiscard]] point_t point_at(float t) const {
        return origin_ + t * direction_;
    }
};

#endif  // RT_RAY_H
