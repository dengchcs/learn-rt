//
// Created by CD on 2022/10/30.
//

#ifndef RT_AABB_HPP
#define RT_AABB_HPP

#include <algorithm>

#include "common.hpp"
#include "ray.hpp"

/**
 * @brief 轴对齐包围盒
 */
class aabb {
    point_t low_;
    point_t high_;

public:
    aabb() {
        const auto nlow = std::numeric_limits<float>::lowest();
        const auto nhigh = std::numeric_limits<float>::max();
        low_ = {nhigh, nhigh, nhigh};
        high_ = {nlow, nlow, nlow};
    }

    aabb(const point_t &low, const point_t &high) : low_(low), high_(high) {}

    /**
     * @brief 包围盒与光线在给定参数范围内是否有交
     */
    [[nodiscard]] bool hit(const ray &r, float tmin, float tmax) const {
        const auto direction = r.direction();
        const auto origin = r.origin();
        for (int i = 0; i < 3; i++) {
            const float invd = 1.0f / direction[i];
            float t0 = (low_[i] - origin[i]) * invd;
            float t1 = (high_[i] - origin[i]) * invd;
            if (invd < 0.0f) {
                std::swap(t0, t1);
            }
            tmin = std::max(tmin, t0);
            tmax = std::min(tmax, t1);
            if (tmin >= tmax) {
                return false;
            }
        }
        return true;
    }

    /**
     * @brief 计算此包围盒与另一包围盒的包围盒
     */
    [[nodiscard]] aabb union_with(const aabb &other) const {
        const auto low = other.low_, high = other.high_;
        const point_t res_low{std::min(low_.x(), low.x()),
                              std::min(low_.y(), low.y()),
                              std::min(low_.z(), low.z())};
        const point_t res_high{std::max(high_.x(), high.x()),
                               std::max(high_.y(), high.y()),
                               std::max(high_.z(), high.z())};
        return {res_low, res_high};
    }

    [[nodiscard]] point_t low() const { return low_; }

    [[nodiscard]] point_t high() const { return high_; }

    [[nodiscard]] int max_extent_dim() const {
        float ext[3];
        for (int i = 0; i < 3; i++) {
            ext[i] = std::abs(low_[i] - high_[i]);
        }
        return std::distance(ext, std::max_element(ext, ext + 3));
    }

    [[nodiscard]] point_t centroid() const { return (low_ + high_) / 2.0; }

    [[nodiscard]] float area() const {
        const auto diagonal = high_ - low_;
        return 2 * (diagonal[0] * (diagonal[1] + diagonal[2]) +
                    diagonal[1] * diagonal[2]);
    }
};

#endif  // RT_AABB_HPP
