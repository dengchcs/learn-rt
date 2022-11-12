#include "bvh.hpp"

#include <algorithm>
#include <execution>
#include <iostream>

bvh::bvh(world_t &world, int start, int end) {
    const auto span = end - start;
    if (span == 1) {
        bounding_ = world[start]->bounding_box();
        object_ = world[start];
        left_ = nullptr, right_ = nullptr;
    } else {
        using phit = std::shared_ptr<hittable>;
        auto sort_world = [&](int dim) {
            if (dim < 0 || dim > 2) {
                std::cerr << "bad dimension: dim = " << dim
                          << ", start & end: " << start << " " << end << "\n";
            }
            std::sort(std::execution::par, world.begin() + start,
                      world.begin() + end, [&](const phit &h1, const phit &h2) {
                          return h1->bounding_box().centroid()[dim] <
                                 h2->bounding_box().centroid()[dim];
                      });
        };
        const int n_split = std::min(span - 1, 12);  // 尝试几次划分
        int min_idx = start + span / 2;
        auto min_cost = std::numeric_limits<double>::max();
        for (int dim = 0; dim < 3; dim++) {
            sort_world(dim);
            for (int i = 1; i <= n_split; i++) {
                const int mid = start + span * i / (n_split + 1);
                const auto bound_l = ::bounding_box(world, start, mid);
                const auto bound_r = ::bounding_box(world, mid, end);
                const double SA = bound_l.area(), SB = bound_r.area();
                const auto cost = (mid - start) * SA + (end - mid) * SB;
                if (cost < min_cost) {
                    min_cost = cost;
                    min_idx = mid;
                    split_axis_ = dim;
                }
            }
        }
        sort_world(split_axis_);
        left_ = std::make_unique<bvh>(world, start, min_idx);
        right_ = std::make_unique<bvh>(world, min_idx, end);
        bounding_ = left_->bounding_.union_with(right_->bounding_);
        object_ = nullptr;
    }
}

std::optional<hit_record> bvh::hit(const ray &r, float tmin, float tmax) const {
    if (!bounding_.hit(r, tmin, tmax)) {
        return std::nullopt;
    }
    if (object_ != nullptr) {
        return object_->hit(r, tmin, tmax);
    }
    auto rec_l = left_->hit(r, tmin, tmax);
    if (rec_l.has_value()) {
        auto rec_r = right_->hit(r, tmin, rec_l->ray_param);
        return rec_r.has_value() ? rec_r : rec_l;
    } else {
        return right_->hit(r, tmin, tmax);
    }
}