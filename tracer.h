//
// Created by CD on 2022/10/30.
//

#ifndef RT_TRACER_H
#define RT_TRACER_H

#include "common.h"
#include "bvh_node.h"
#include "camera.h"
#include "hittable.h"
#include "material.h"
#include <execution>
#include <iostream>
#include <fstream>

#ifndef STB_IMAGE_WRITE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#endif

#include "deps/stb_image_write.h"

class tracer {
public:
    struct config {
        int width, height;
        int samples_per_pixel;
        int max_depth;
        bool use_bvh, parallel;
    };

    tracer(const config &conf, const camera &cam) : config_(conf), cam_(cam) {

    }

    ~tracer() { delete data_; }

private:
    config config_;
    camera cam_;
    unsigned char *data_{nullptr};

private:
    template<typename T>
    [[nodiscard]] color_t ray_color(const ray &r, const T &target, int depth) const {
        if (depth <= 0) {
            return {0, 0, 0};
        }
        std::optional<hit_record> record = std::nullopt;
        if constexpr (std::is_same_v<T, bvh_node>) {
            record = target.hit(r, 0.001, g_infinity);
        } else if constexpr (std::is_same_v<T, world_t>) {
            record = hit(target, r, 0.001, g_infinity);
        }
        if (record.has_value()) {
            ray scattered;
            color_t attenuation;
            if (record->pmat->scatter(r, record.value(), attenuation, scattered)) {
                return attenuation * ray_color(scattered, target, depth - 1);
            }
            return color_t{0, 0, 0};
        }
        float t = (r.direction().y() + 1.0f) / 2.0f;
        return (1.0f - t) * color_t{1.0, 1.0, 1.0} + t * color_t{0.5, 0.7, 1.0};
    }

    /**
     * @brief 写入一个像素, 图像左上角为起点
     */
    void write_color(int row, int col, const color_t &pixel) {
        auto r = pixel.x(), g = pixel.y(), b = pixel.z();
        const float scale = 1.0f / (float) config_.samples_per_pixel;
        r = std::sqrt(r * scale);   // Gamma Correction (gamma = 0.5), gamma越小图片越亮
        g = std::sqrt(g * scale);
        b = std::sqrt(b * scale);

        int index = (col + row * config_.width) * 3;
        data_[index + 0] = static_cast<int>(256 * clamp(r, 0, 0.999));
        data_[index + 1] = static_cast<int>(256 * clamp(g, 0, 0.999));
        data_[index + 2] = static_cast<int>(256 * clamp(b, 0, 0.999));
    }

    template<typename T>
    void trace_sequential(const T &target, const std::string &path) {
        std::cout << "writing data to " << path << ".\n";
        auto start = std::chrono::steady_clock::now();
        delete data_;
        data_ = new unsigned char[config_.width * config_.height * 3];
        for (int j = config_.height - 1; j >= 0; j--) {
            std::cout << "lines remaining: " << j << "\n";
            for (int i = 0; i < config_.width; i++) {
                color_t color{0, 0, 0};
                for (int s = 0; s < config_.samples_per_pixel; s++) {
                    const float u = ((float) i + random_float()) / (float) config_.width;
                    const float v = ((float) j + random_float()) / (float) config_.height;
                    ray r = cam_.get_ray(u, v);
                    color += ray_color(r, target, config_.max_depth);
                }
                const int row = config_.height - 1 - j, col = i;
                write_color(row, col, color);
            }
        }
        stbi_write_png(path.c_str(), config_.width, config_.height, 3, data_, config_.width * 3);
        auto end = std::chrono::steady_clock::now();
        std::cout << "\ndone in " << std::chrono::duration<double>(end - start).count() << "s.\n";
    }

    template<typename T>
    void trace_parallel(const T &target, const std::string &path) {
        std::cout << "writing data to " << path << ".\n";
        auto start = std::chrono::steady_clock::now();
        delete data_;
        data_ = new unsigned char[config_.width * config_.height * 3];

        std::vector<int> range(config_.width * config_.height);
        std::generate(std::execution::par, range.begin(), range.end(), [n = 0]() mutable { return n++; });
        std::for_each(
                std::execution::par,
                range.begin(), range.end(),
                [&](int index) {
                    const int row = index / config_.width;
                    const int col = index % config_.width;
                    color_t color{0, 0, 0};
                    for (int s = 0; s < config_.samples_per_pixel; s++) {
                        const float u = ((float) col + random_float()) / (float) config_.width;
                        const float v = ((float) (config_.height - 1 - row) + random_float()) / (float) config_.height;
                        ray r = cam_.get_ray(u, v);
                        color += ray_color(r, target, config_.max_depth);
                    }
                    write_color(row, col, color);
                });
        stbi_write_png(path.c_str(), config_.width, config_.height, 3, data_, config_.width * 3);
        auto end = std::chrono::steady_clock::now();
        std::cout << "\ndone in " << std::chrono::duration<double>(end - start).count() << "s.\n";
    }

public:
    void trace(const world_t &world, const std::string &path) {
        if (config_.parallel) {
            if (config_.use_bvh) {
                bvh_node bvh{world, 0, world.size()};
                trace_parallel(bvh, path);
            } else {
                trace_parallel(world, path);
            }
        } else {
            if (config_.use_bvh) {
                bvh_node bvh{world, 0, world.size()};
                trace_sequential(bvh, path);
            } else {
                trace_sequential(world, path);
            }
        }
    }
};

#endif //RT_TRACER_H
