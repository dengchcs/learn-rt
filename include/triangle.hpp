#ifndef RT_TRIANGLE_HPP
#define RT_TRIANGLE_HPP

#include <array>
#include <iostream>

#include "hittable.hpp"

class triangle : public hittable {
    std::array<point_t, 3> vertices_;
    unit_vec3 normal_;      // 顶点按右手方向旋转得到的法向
    float dist_to_origin_;  // A*x+B*y+C*z+D=0中的D
    std::shared_ptr<material> pmat_;
    std::array<tex_coords_t, 3> tex_coords_;
    std::array<vec3_t, 3> edges_;
    float area2_;

public:
    triangle(const point_t &point0, const point_t &point1, const point_t &point2,
             const std::array<tex_coords_t, 3> &tex_coords, std::shared_ptr<material> pmat)
        : vertices_{point0, point1, point2}, pmat_(std::move(pmat)), tex_coords_{tex_coords} {
        normal_ = (point1 - point0).cross(point2 - point1).normalized();
        if (normal_.len_sq() == 0) {
            std::cerr << "bad triangle: collinear edges\n";
        }
        dist_to_origin_ = -normal_.dot(point0);
        edges_ = {point2 - point1, point0 - point2, point1 - point0};
        area2_ = edges_[0].cross(edges_[1]).len();
    }

private:
    [[nodiscard]] point_t vertex(int i) const { return vertices_.at(i % 3); }

public:
    [[nodiscard]] hit_res_t hit(const ray &r, float tmin, float tmax) const override;

    [[nodiscard]] aabb bounding_box() const override;
};

#endif  // RT_TRIANGLE_HPP
