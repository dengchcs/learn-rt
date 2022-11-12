#include "triangle.hpp"

#include "aabb.hpp"
#include "ray.hpp"

std::optional<hit_record> triangle::hit(const ray &r, float tmin, float tmax) const {
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

aabb triangle::bounding_box() const {
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