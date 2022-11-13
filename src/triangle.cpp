#include "triangle.hpp"

#include <array>

#include "aabb.hpp"
#include "ray.hpp"

hit_res_t triangle::hit(const ray &r, float tmin, float tmax) const {
    const float divisor = normal_.dot(r.direction());
    if (divisor == 0) {
        return std::nullopt;
    }
    const float t = -(normal_.dot(r.origin()) + dist_to_origin_) / divisor;
    if (t < tmin || t > tmax) {
        return std::nullopt;
    }

    const point_t point = r.point_at(t);
    std::array<float, 3> bary{};
    for (int i = 0; i < 3; i++) {
        bary.at(i) = edges_.at(i).cross(point - vertex(i + 1)).len() / area2_;
        if (bary.at(i) < 0 || bary.at(i) > 1) {
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
    rec.tex_coords = {0, 0};
    for (int i = 0; i < 3; i++) {
        rec.tex_coords.at(0) += bary.at(i) * tex_coords_.at(i).at(0);
        rec.tex_coords.at(1) += bary.at(i) * tex_coords_.at(i).at(1);
    }
    return rec;
}

aabb triangle::bounding_box() const {
    const auto inf = std::numeric_limits<float>::max();
    std::array<float, 3> coord_min = {inf, inf, inf};
    std::array<float, 3> coord_max = {-inf, -inf, -inf};
    for (auto &&vertex : vertices_) {
        for (int k = 0; k < 3; k++) {  // x,y,z维度
            coord_min.at(k) = std::min(coord_min.at(k), vertex[k]);
            coord_max.at(k) = std::max(coord_max.at(k), vertex[k]);
        }
    }
    for (int k = 0; k < 3; k++) {
        coord_min.at(k) -= 0.0001F;
        coord_max.at(k) += 0.0001F;
    }
    return aabb{vec3_t{coord_min.data()}, vec3_t{coord_max.data()}};
}