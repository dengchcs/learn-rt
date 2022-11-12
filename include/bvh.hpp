//
// Created by CD on 2022/10/30.
//

#ifndef RT_BVH_HPP
#define RT_BVH_HPP

#include <memory>
#include <optional>

#include "aabb.hpp"
#include "hittable.hpp"


class bvh {
    std::unique_ptr<bvh> left_, right_;
    int split_axis_{0};
    aabb bounding_;
    std::shared_ptr<hittable> object_;

public:
    bvh(world_t &world, int start, int end);

    [[nodiscard]] std::optional<hit_record> hit(const ray &r, float tmin,
                                                float tmax) const;
};

#endif  // RT_BVH_HPP
