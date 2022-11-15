#ifndef RT_TRACER_HPP
#define RT_TRACER_HPP

#include <execution>
#include <iostream>
#include <numeric>
#include <string>
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
    void render_single_pixel(const T &target, int row, int col) {
        color_t color{0, 0, 0};
        for (int sample = 0; sample < config_.samples_per_pixel; sample++) {
            const float u = ((float)col + random_float()) / (float)config_.width;
            const float v =
                ((float)(config_.height - 1 - row) + random_float()) / (float)config_.height;
            ray ray = cam_.get_ray(u, v);
            color += cast_ray(ray, target, config_.max_depth);
        }
        write_color(row, col, color);
    }

    template <typename T>
    void trace_unified(const T &target, const std::string &path) {
        std::cout << "scene rendering: started...\n";
        std::cout << "\twriting data to " << path << ".\n";
        auto start = std::chrono::steady_clock::now();

        if (config_.parallel) {
            std::vector<int> range(config_.width * config_.height);
            std::iota(range.begin(), range.end(), 0);
            std::for_each(std::execution::par, range.begin(), range.end(), [&](int index) {
                const int row = index / config_.width;
                const int col = index % config_.width;
                render_single_pixel(target, row, col);
            });
        } else {
            for (int row = 0; row < config_.height; row++) {
                for (int col = 0; col < config_.width; col++) {
                    render_single_pixel(target, row, col);
                }
            }
        }
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
            trace_unified(bvh, path);
        } else {
            trace_unified(world, path);
        }
    }
};

#endif  // RT_TRACER_HPP
