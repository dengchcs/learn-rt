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
#include <QDebug>

#define STB_IMAGE_WRITE_IMPLEMENTATION

#include "deps/stb_image_write.h"

class tracer {
public:
    struct config {
        int width, height;
        int samples_per_pixel;
        int max_depth;
        bool use_bvh;
    };

    tracer(const config &conf, const camera &cam) : config_(conf), cam_(cam) {

    }

private:
    config config_;
    camera cam_;
    unsigned char *data_{nullptr};
private:
    [[nodiscard]] color_t ray_color(const ray &r, const bvh_node &bvh, int depth) const {
        if (depth <= 0) {
            return {0, 0, 0};
        }
        auto record = bvh.hit(r, 0.001, g_infinity);    // 忽略距离太近的光线
        if (record.has_value()) {
            ray scattered;
            color_t attenuation;
            if (record->pmat->scatter(r, record.value(), attenuation, scattered)) {
                return attenuation * ray_color(scattered, bvh, depth - 1);
            }
            return color_t{0, 0, 0};
        }
        float t = (r.direction().y() + 1.0f) / 2.0f;
        return (1.0f - t) * color_t{1.0, 1.0, 1.0} + t * color_t{0.5, 0.7, 1.0};
    }

    void write_color(std::ofstream &file, color_t pixel) const {
        auto r = pixel.x(), g = pixel.y(), b = pixel.z();

        const float scale = 1.0f / config_.samples_per_pixel;
        r = std::sqrt(r * scale);   // Gamma Correction (gamma = 0.5), gamma越小图片越亮
        g = std::sqrt(g * scale);
        b = std::sqrt(b * scale);

        file << static_cast<int>(256 * clamp(r, 0, 0.999)) << ' '
             << static_cast<int>(256 * clamp(g, 0, 0.999)) << ' '
             << static_cast<int>(256 * clamp(b, 0, 0.999)) << '\n';
    }

    void write_color(int row, int col, color_t pixel) {
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

public:
    void trace(const world_t &world, const std::string &path) {
        qDebug() << "writing data_ to " << path.c_str() << ".\n";
        auto start = std::chrono::steady_clock::now();
        std::ofstream file(path);
        file << "P3\n" << config_.width << " " << config_.height << "\n255\n";
        bvh_node bvh{world, 0, world.size()};
        for (int j = config_.height - 1; j >= 0; j--) {
            qDebug() << "lines remaining: " << j;
            for (int i = 0; i < config_.width; i++) {
                color_t color{0, 0, 0};
                for (int s = 0; s < config_.samples_per_pixel; s++) {
                    const float u = ((float) i + random_float()) / (float) config_.width;
                    const float v = ((float) j + random_float()) / (float) config_.height;
                    ray r = cam_.get_ray(u, v);
                    color += ray_color(r, bvh, config_.max_depth);
                }
                write_color(file, color);
            }
        }
        file.close();
        auto end = std::chrono::steady_clock::now();
        qDebug() << "\ndone in " << std::chrono::duration<double>(end - start).count() << "s.";
    }

    void trace_png(const world_t &world, const std::string &path) {
        qDebug() << "writing data to " << path.c_str() << ".\n";
        auto start = std::chrono::steady_clock::now();
        delete data_;
        data_ = new unsigned char[config_.width * config_.height * 3];
        bvh_node bvh{world, 0, world.size()};
        for (int j = config_.height - 1; j >= 0; j--) {
            qDebug() << "lines remaining: " << j;
            for (int i = 0; i < config_.width; i++) {
                color_t color{0, 0, 0};
                for (int s = 0; s < config_.samples_per_pixel; s++) {
                    const float u = ((float) i + random_float()) / (float) config_.width;
                    const float v = ((float) j + random_float()) / (float) config_.height;
                    ray r = cam_.get_ray(u, v);
                    color += ray_color(r, bvh, config_.max_depth);
                }
                const int row = config_.height - 1 - j, col = i;
                write_color(row, col, color);
            }
        }
        stbi_write_png(path.c_str(), config_.width, config_.height, 3, data_, config_.width * 3);
        auto end = std::chrono::steady_clock::now();
        qDebug() << "\ndone in " << std::chrono::duration<double>(end - start).count() << "s.";
    }
};

#endif //RT_TRACER_H
