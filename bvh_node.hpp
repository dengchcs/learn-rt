//
// Created by CD on 2022/10/30.
//

#ifndef RT_BVH_NODE_HPP
#define RT_BVH_NODE_HPP

#include "aabb.hpp"
#include "common.hpp"
#include "hittable.hpp"
#include "utils.hpp"
#include <algorithm>
#include <iostream>

/**
 * BVH树, 划分轴是随机选取的
 */
class bvh_node : public hittable {
    std::shared_ptr<hittable> left_, right_;
    aabb bounding_;
public:
    bvh_node() = default;

    /**
     * @brief 构建包含场景给定范围 [start, end) 的物体的BVH树<br>
     * 因为要对物体排序, 所以world参数不是const的
     */
    bvh_node(world_t &world, std::size_t start, std::size_t end) {
        aabb bounds = world[start]->bounding_box();
        for (auto i = start; i < end; i++) {
            bounds = bounds.union_with(world[i]->bounding_box());
        }
        const int dim = bounds.max_extent_dim();
        auto comparator = [dim](const std::shared_ptr<hittable> &h1, const std::shared_ptr<hittable> &h2) {
            const auto c1 = h1->bounding_box().centroid(), c2 = h2->bounding_box().centroid();
            return c1[dim] < c2[dim];
        };
        const auto span = end - start;
        if (span == 1) {
            left_ = right_ = world[start];
        } else if (span == 2) {
            left_ = world[start], right_ = world[start + 1];
            if (!comparator(left_, right_)) {
                std::swap(left_, right_);
            }
        } else {
            std::sort(world.begin() + start, world.begin() + end, comparator);
            const auto mid = start + span / 2;
            left_ = std::make_shared<bvh_node>(world, start, mid);
            right_ = std::make_shared<bvh_node>(world, mid, end);
        }
        const auto box_l = left_->bounding_box(), box_r = right_->bounding_box();
        bounding_ = box_l.union_with(box_r);
    }

public:
    [[nodiscard]] std::optional<hit_record> hit(const ray &r, float tmin, float tmax) const override {
        if (!bounding_.hit(r, tmin, tmax)) {
            return std::nullopt;
        }
        auto rec_l = left_->hit(r, tmin, tmax);
        if (rec_l.has_value()) {
            auto rec_r = right_->hit(r, tmin, rec_l->ray_param);
            return rec_r.has_value() ? rec_r : rec_l;
        } else {
            return right_->hit(r, tmin, tmax);
        }
    }

    [[nodiscard]] aabb bounding_box() const override {
        return bounding_;
    }
};

#endif //RT_BVH_NODE_HPP
