#ifndef RT_TRIANGLE_HPP
#define RT_TRIANGLE_HPP

#include <array>
#include <iostream>

#include "hittable.hpp"

class triangle : public hittable {
    std::array<point_t, 3> vertices_;
    unit_vec3 normal_;      // 顶点按右手方向旋转得到的法向
    float dist_to_origin_;  // A*x+B*y+C*z+D=0中的D
    float area2_;           // 面积的两倍
    std::shared_ptr<material> pmat_;
    std::array<tex_coords_t, 3> tex_coords_;
    std::array<vec3_t, 3> edges_;

public:
    triangle(const point_t &point0, const point_t &point1, const point_t &point2,
             const tex_coords_t &tex_coords0, const tex_coords_t &tex_coords1, const tex_coords_t &tex_coords2,
             std::shared_ptr<material> pmat)
        : vertices_{point0, point1, point2}, pmat_(std::move(pmat)), tex_coords_{tex_coords0, tex_coords1, tex_coords2} {
        normal_ = (point1 - point0).cross(point2 - point1).normalized();
        if (normal_.len_sq() == 0) {
            std::cerr << "bad triangle: collinear edges\n";
        }
        dist_to_origin_ = -normal_.dot(point0);
        edges_ = {point2 - point1, point0 - point2, point1 - point0};
        area2_ = edges_[0].cross(edges_[1]).len();
    }

private:
    [[nodiscard]] point_t vertex(int idx) const { return vertices_.at(idx % 3); }

public:
    [[nodiscard]] hit_res_t hit(const ray &r, float tmin, float tmax) const override;

    [[nodiscard]] aabb bounding_box() const override;
};

#endif  // RT_TRIANGLE_HPP
