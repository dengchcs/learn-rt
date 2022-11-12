//
// Created by CD on 2022/10/26.
//

#ifndef RT_SPHERE_HPP
#define RT_SPHERE_HPP

#include <utility>

#include "aabb.hpp"
#include "common.hpp"
#include "hittable.hpp"
#include "ray.hpp"

class material;

class sphere : public hittable {
    point_t center_;
    float radius_;

public:
    std::shared_ptr<material> pmat_;

public:
    [[nodiscard]] point_t center() const { return center_; }

    [[nodiscard]] float radius() const { return radius_; }

private:
    /**
     * @brief 计算球面一点的参数坐标, 参数范围为[0, 1] * [0, 1]
     */
    void uv_at(const point_t &point, float &u, float &v) const {
        const auto p_unit = (point - center_) / radius_;
        const float theta = std::acos(-p_unit.y());
        const float phi = std::atan2(-p_unit.z(), p_unit.x()) + g_pi;
        u = phi / (2.0f * g_pi);
        v = theta / g_pi;
    }

public:
    sphere(const point_t &center, float radius, std::shared_ptr<material> m)
        : center_(center), radius_(radius), pmat_(std::move(m)) {}

    [[nodiscard]] std::optional<hit_record> hit(const ray &r, float tmin,
                                                float tmax) const override {
        const vec3_t oc = r.origin() - center_;
        const float b_half = oc.dot(r.direction());
        const float c = oc.len_sq() - radius_ * radius_;
        const float discriminant = b_half * b_half - c;
        if (discriminant < 0) {
            return std::nullopt;
        }

        const float sqrtd = std::sqrt(discriminant);
        float root = -b_half - sqrtd;
        if (root < tmin || root > tmax) {
            root = -b_half + sqrtd;
            if (root < tmin || root > tmax) {
                return std::nullopt;
            }
        }

        hit_record record;
        record.ray_param = root;
        record.point = r.point_at(record.ray_param);
        record.normal = unit_vec3(record.point - center_);
        record.outside =
            (r.direction().dot(record.normal) < 0) == (radius_ > 0);
        if (record.outside == (radius_ < 0)) {
            record.normal = -record.normal;
        }
        uv_at(record.point, record.u, record.v);
        record.pmat = pmat_;
        return record;
    }

    [[nodiscard]] aabb bounding_box() const override {
        const float absr = std::abs(radius_);
        const vec3_t disp{absr, absr, absr};
        return aabb{center_ - disp, center_ + disp};
    }
};

#endif  // RT_SPHERE_HPP
