//
// Created by CD on 2022/10/26.
//

#ifndef RT_SPHERE_H
#define RT_SPHERE_H

#include <utility>

#include "common.h"
#include "hittable.h"
#include "ray.h"

class material;

class sphere : public hittable {
    point_t center_;
    float radius_;
public:
    std::shared_ptr<material> pmat_;
public:
    [[nodiscard]] point_t center() const {
        return center_;
    }

    [[nodiscard]] float radius() const {
        return radius_;
    }

public:
    sphere(const point_t &center, float radius, std::shared_ptr<material> m)
            : center_(center), radius_(radius), pmat_(std::move(m)) {

    }

    [[nodiscard]] std::optional<hit_record> hit(const ray &r, float tmin, float tmax) const override {
        const vec3_t oc = r.origin() - center_;
        const float a = 1;
        const float b = 2.0f * QVector3D::dotProduct(oc, r.direction());
        const float c = oc.lengthSquared() - radius_ * radius_;
        const float discriminant = b * b - 4 * a * c;
        if (discriminant < 0) {
            return std::nullopt;
        }

        const float sqrtd = std::sqrt(discriminant);
        float root = (-b - sqrtd) / (2.0f * a);
        if (root < tmin || root > tmax) {
            root = (-b + sqrtd) / (2.0f * a);
            if (root < tmin || root > tmax) {
                return std::nullopt;
            }
        }

        hit_record record;
        record.ray_param = root;
        record.point = r.point_at(record.ray_param);
        record.normal = unit_vec3{record.point - center_};
        if (radius_ < 0) record.normal *= -1;
        record.outside = QVector3D::dotProduct(r.direction(), record.normal) < 0;
        if (!record.outside) record.normal *= -1;
        record.pmat = pmat_;
        return record;
    }

    [[nodiscard]] std::optional<aabb> bounding_box() const override {
        const float absr = std::abs(radius_);
        const vec3_t disp{absr, absr, absr};
        return aabb{
                center_ - disp,
                center_ + disp
        };
    }
};

#endif //RT_SPHERE_H
