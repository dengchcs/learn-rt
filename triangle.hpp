//
// Created by CD on 2022/11/1.
//

#ifndef RT_TRIANGLE_HPP
#define RT_TRIANGLE_HPP

#include "hittable.hpp"
#include <iostream>
#include <limits>

class triangle : public hittable {
    point_t vertices_[3];
    unit_vec3 normal_;  // 顶点按右手方向旋转得到的法向
    float dist_to_origin_;  // A*x+B*y+C*z+D=0中的D
    std::shared_ptr<material> pmat_;
    using tex_coords_t = std::pair<float, float>;
    tex_coords_t tex_coords_[3];

public:
    triangle(const point_t &p0, const point_t &p1, const point_t &p2, float tex[6], std::shared_ptr<material> pmat)
            : vertices_{p0, p1, p2}, tex_coords_{{tex[0], tex[1]},
                                                 {tex[2], tex[3]},
                                                 {tex[4], tex[5]}}, pmat_(std::move(pmat)) {
        normal_ = (p1 - p0).cross(p2 - p1).normalized();
        if (normal_.len_sq() == 0) {
            std::cerr << "bad triangle: collinear edges\n";
        }
        dist_to_origin_ = -normal_.dot(p0);
    }

private:
    [[nodiscard]] point_t vertex(int i) const { return vertices_[i % 3]; }

public:
    [[nodiscard]] std::optional<hit_record> hit(const ray &r, float tmin, float tmax) const override {
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
            const vec3_t ei = vertex(i + 2) - vertex(i + 1);
            const vec3_t ep = point - vertex(i + 1);
            bary[i] = ei.cross(ep).len();
        }
        const float area = (vertices_[1] - vertices_[0]).cross(vertices_[2] - vertices_[0]).len();
        const float diff = std::abs(bary[0] + bary[1] + bary[2] - area);
        if (diff > std::numeric_limits<float>::epsilon()) {
            return std::nullopt;
        }
        for (auto &&b: bary) {
            b /= area;
            if (b < 0 || b > 1) return std::nullopt;
        }

        hit_record rec;
        rec.ray_param = t;
        rec.point = point;
        rec.outside = true;
        rec.normal = divisor < 0 ? normal_ : unit_vec3{-normal_};
        rec.pmat = pmat_;
        rec.u = rec.v = 0;
        for (int i = 0; i < 3; i++) {
            rec.u += bary[i] * tex_coords_[i].first;
            rec.v += bary[i] * tex_coords_[i].second;
        }
        return rec;
    }

    [[nodiscard]] std::optional<aabb> bounding_box() const override {
        const auto inf = std::numeric_limits<float>::max();
        float coord_min[3] = {inf, inf, inf};
        float coord_max[3] = {-inf, -inf, -inf};
        for (auto &&vertex: vertices_) {
            for (int k = 0; k < 3; k++) {   // x,y,z维度
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

#endif //RT_TRIANGLE_HPP
