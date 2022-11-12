//
// Created by CD on 2022/11/1.
//

#ifndef RT_TRIANGLE_HPP
#define RT_TRIANGLE_HPP

#include <iostream>
#include <limits>

#include "hittable.hpp"

class triangle : public hittable {
    std::array<point_t, 3> vertices_;
    unit_vec3 normal_;      // 顶点按右手方向旋转得到的法向
    float dist_to_origin_;  // A*x+B*y+C*z+D=0中的D
    std::shared_ptr<material> pmat_;
    using tex_coords_t = std::pair<float, float>;
    std::array<tex_coords_t, 3> tex_coords_;
    std::array<vec3_t, 3> edges_;
    float area2_;

public:
    triangle(const point_t &p0, const point_t &p1, const point_t &p2,
             float tex[6], std::shared_ptr<material> pmat)
        : vertices_{p0, p1, p2},
          pmat_(std::move(pmat)),
          tex_coords_{tex_coords_t{tex[0], tex[1]},
                      {tex[2], tex[3]},
                      {tex[4], tex[5]}} {
        normal_ = (p1 - p0).cross(p2 - p1).normalized();
        if (normal_.len_sq() == 0) {
            std::cerr << "bad triangle: collinear edges\n";
        }
        dist_to_origin_ = -normal_.dot(p0);
        edges_ = {p2 - p1, p0 - p2, p1 - p0};
        area2_ = edges_[0].cross(edges_[1]).len();
    }

private:
    [[nodiscard]] point_t vertex(int i) const { return vertices_[i % 3]; }

public:
    [[nodiscard]] std::optional<hit_record> hit(const ray &r, float tmin,
                                                float tmax) const override {
        const float divisor = normal_.dot(r.direction());
        if (divisor == 0) {
            return std::nullopt;
        }
        const float t = -(normal_.dot(r.origin()) + dist_to_origin_) / divisor;
        if (t < tmin || t > tmax) {
            return std::nullopt;
        }

        const point_t point = r.point_at(t);
        float bary[3];
        for (int i = 0; i < 3; i++) {
            bary[i] = edges_[i].cross(point - vertex(i + 1)).len() / area2_;
            if (bary[i] < 0 || bary[i] > 1) {
                return std::nullopt;
            }
        }
        const float diff = std::abs(bary[0] + bary[1] + bary[2] - 1);
        if (diff > std::numeric_limits<float>::epsilon()) {
            return std::nullopt;
        }

        hit_record rec;
        rec.ray_param = t;
        rec.point = point;
        rec.outside = divisor < 0;
        rec.normal = divisor < 0 ? normal_ : -normal_;
        rec.pmat = pmat_;
        rec.u = rec.v = 0;
        for (int i = 0; i < 3; i++) {
            rec.u += bary[i] * tex_coords_[i].first;
            rec.v += bary[i] * tex_coords_[i].second;
        }
        return rec;
    }

    [[nodiscard]] aabb bounding_box() const override {
        const auto inf = std::numeric_limits<float>::max();
        float coord_min[3] = {inf, inf, inf};
        float coord_max[3] = {-inf, -inf, -inf};
        for (auto &&vertex : vertices_) {
            for (int k = 0; k < 3; k++) {  // x,y,z维度
                coord_min[k] = std::min(coord_min[k], vertex[k]);
                coord_max[k] = std::max(coord_max[k], vertex[k]);
            }
        }
        for (int k = 0; k < 3; k++) {
            coord_min[k] -= 0.0001;
            coord_max[k] += 0.0001;
        }
        return aabb{vec3_t{coord_min}, vec3_t{coord_max}};
    }
};

#endif  // RT_TRIANGLE_HPP
