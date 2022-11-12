#ifndef RT_RECTANGLE_HPP
#define RT_RECTANGLE_HPP

#include <iostream>

#include "hittable.hpp"

class rectangle : public hittable {
    point_t corner_;
    vec3_t edge_u_;
    vec3_t edge_v_;
    unit_vec3 normal_;
    std::shared_ptr<material> pmat_;

public:
    rectangle(const point_t &p, const vec3_t &edgeu, const vec3_t &edgev,
              std::shared_ptr<material> pmat)
        : corner_(p), edge_u_(edgeu), edge_v_(edgev), pmat_(std::move(pmat)) {
        normal_ = edgeu.cross(edgev).normalized();
        if (normal_.len_sq() == 0) {
            std::cerr << "bad rectangle: collinear edge\n";
        }
        edge_v_ = normal_.cross(edge_u_);
    }

    [[nodiscard]] std::optional<hit_record> hit(const ray &r, float tmin,
                                                float tmax) const override;

    [[nodiscard]] aabb bounding_box() const override;
};

#endif  // RT_RECTANGLE_HPP
