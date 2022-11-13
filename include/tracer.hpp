#ifndef RT_TRACER_HPP
#define RT_TRACER_HPP

#include <execution>
#include <iostream>
#include <numeric>
#include <vector>

#include "bvh.hpp"
#include "camera.hpp"
#include "common.hpp"
#include "hittable.hpp"
#include "material.hpp"
#include "stb_image_write.h"

class tracer {
public:
    struct config {
        int width, height;
        int samples_per_pixel;
        int max_depth;
        bool use_bvh, parallel;
        color_t background;
    };

    tracer(const config &conf, const camera &cam)
        : config_(conf), cam_(cam), data_(config_.width * config_.height * 3) {}

    // ~tracer() { delete[] data_; }

private:
    config config_;
    camera cam_;
    // unsigned char *data_{nullptr};
    std::vector<unsigned char> data_;
    /**
     * @brief 写入一个像素, 图像左上角为起点
     */
    void write_color(int row, int col, const color_t &pixel) {
        auto r = pixel.x();
        auto g = pixel.y();
        auto b = pixel.z();
        const float scale = 1.0F / (float)config_.samples_per_pixel;
        r = std::sqrt(r * scale);  // Gamma Correction (gamma = 0.5), gamma越小图片越亮
        g = std::sqrt(g * scale);
        b = std::sqrt(b * scale);

        constexpr int max_color = 255;
        int index = (col + row * config_.width) * 3;
        data_[index + 0] = (int)(max_color * clamp(r, 0, 1));
        data_[index + 1] = (int)(max_color * clamp(g, 0, 1));
        data_[index + 2] = (int)(max_color * clamp(b, 0, 1));
    }

    template <typename T>
    [[nodiscard]] color_t cast_ray(const ray &r, const T &target, int depth) const {
        if (depth <= 0) {
            return {0, 0, 0};
        }
        hit_res_t record = std::nullopt;
        constexpr float ray_nearest_t = 0.001F;
        if constexpr (std::is_same_v<T, bvh>) {
            record = target.hit(r, ray_nearest_t, g_infinity);
        } else if constexpr (std::is_same_v<T, world_t>) {
            record = hit(target, r, ray_nearest_t, g_infinity);
        } else {
            std::cerr << "unknown target type\n";
        }
        if (!record.has_value()) {
            return config_.background;
        }
        ray scattered;
        color_t attenuation;
        const color_t emitted = record->pmat->emit(record->tex_coords, record->point);
        const bool scatter = record->pmat->scatter(r, record.value(), attenuation, scattered);
        if (!scatter) {
            return emitted;
        }
        return emitted + attenuation * cast_ray(scattered, target, depth - 1);
    }

    template <typename T>
    void trace_sequential(const T &target, const std::string &path) {
        std::cout << "scene rendering: started...\n";
        std::cout << "\twriting data to " << path << ".\n";
        auto start = std::chrono::steady_clock::now();
        for (int j = config_.height - 1; j >= 0; j--) {
            for (int i = 0; i < config_.width; i++) {
                color_t color{0, 0, 0};
                for (int s = 0; s < config_.samples_per_pixel; s++) {
                    const float u = ((float)i + random_float()) / (float)config_.width;
                    const float v = ((float)j + random_float()) / (float)config_.height;
                    ray r = cam_.get_ray(u, v);
                    color += cast_ray(r, target, config_.max_depth);
                }
                const int row = config_.height - 1 - j;
                const int col = i;
                write_color(row, col, color);
            }
        }
        stbi_write_png(path.c_str(), config_.width, config_.height, 3, data_.data(),
                       config_.width * 3);
        auto end = std::chrono::steady_clock::now();
        std::cout << "scene rendering: done in "
                  << std::chrono::duration<double>(end - start).count() << "s.\n\n";
    }

    template <typename T>
    void trace_parallel(const T &target, const std::string &path) {
        std::cout << "scene rendering: started...\n";
        std::cout << "\twriting data to " << path << ".\n";
        auto start = std::chrono::steady_clock::now();

        std::vector<int> range(config_.width * config_.height);
        std::iota(range.begin(), range.end(), 0);
        std::for_each(std::execution::par, range.begin(), range.end(), [&](int index) {
            const int row = index / config_.width;
            const int col = index % config_.width;
            color_t color{0, 0, 0};
            for (int s = 0; s < config_.samples_per_pixel; s++) {
                const float u = ((float)col + random_float()) / (float)config_.width;
                const float v =
                    ((float)(config_.height - 1 - row) + random_float()) / (float)config_.height;
                ray r = cam_.get_ray(u, v);
                color += cast_ray(r, target, config_.max_depth);
            }
            write_color(row, col, color);
        });
        stbi_write_png(path.c_str(), config_.width, config_.height, 3, data_.data(),
                       config_.width * 3);
        auto end = std::chrono::steady_clock::now();
        std::cout << "scene rendering: done in "
                  << std::chrono::duration<double>(end - start).count() << "s.\n\n";
    }

public:
    void trace(const world_t &world, const std::string &path) {
        if (config_.use_bvh) {
            std::cout << "BVH building: started...\n";
            auto start = std::chrono::steady_clock::now();
            world_t world_cp = world;
            bvh bvh{world_cp, 0, (int)world_cp.size()};
            auto end = std::chrono::steady_clock::now();
            std::cout << "BVH building: done in "
                      << std::chrono::duration<double>(end - start).count() << "s.\n\n";
            if (config_.parallel) {
                trace_parallel(bvh, path);
            } else {
                trace_sequential(bvh, path);
            }
        } else {
            if (config_.parallel) {
                trace_parallel(world, path);
            } else {
                trace_sequential(world, path);
            }
        }
    }
};

#endif  // RT_TRACER_HPP
