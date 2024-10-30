#ifndef RT_TRACER_HPP
#define RT_TRACER_HPP

#include <cmath>
#include <cstdlib>
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
        // color_t background;
        // std::string envlight_path;
    };

    struct envlight {
        unsigned char *data;
        int width, height, channels;
        envlight(const std::string &path) {
            data = stbi_load(path.c_str(), &width, &height, &channels, 0);
            if (!data) {
                std::cerr << "failed to load envlight: " << path << '\n';
                std::abort();
            }
        }
        ~envlight() { stbi_image_free(data); }
        color_t sample(float u, float v) const {
            const int x = std::clamp((int)(u * width), 0, width - 1);
            const int y = std::clamp((int)(v * height), 0, height - 1);
            const int index = (x + y * width) * channels;
            return {data[index] / 255.0F, data[index + 1] / 255.0F, data[index + 2] / 255.0F};
        }
    };

    tracer(const config &conf, const camera &cam, const std::string& envlight_path)
        : config_(conf), cam_(cam), data_(config_.width * config_.height * 3), envlight_(envlight_path){}

private:
    config config_;
    camera cam_;
    std::vector<unsigned char> data_;
    envlight envlight_;


    /**
     * @brief 写入一个像素, 图像左上角为起点
     */
    void write_color(int row, int col, const color_t &pixel) {
        constexpr int max_color = 255;
        const int index = (col + row * config_.width) * 3;
        for (int i = 0; i < 3; i++) {
            // Gamma Correction (gamma = 0.5), gamma越小图片越亮
            const auto corrected = std::sqrt(pixel[i]);
            data_[index + i] = (int)(max_color * clamp(corrected, 0, 1));
        }
    }

    /**
     * @brief 向场景中"发射"一条光线, 追踪其反射/折射, 并计算光线的颜色
     *
     * @tparam T 场景聚合体的类型, 必须是: world_t 或 bvh
     * @param r 光线定义
     * @param target 场景聚合体
     * @param depth 递归深度, 为0时退出递归
     * @return color_t 光线追踪得到的颜色
     */
    template <typename T>
    [[nodiscard]] color_t cast_ray(const ray &r, const T &target, int depth) const {
        if (depth <= 0) {
            return {0, 0, 0};
        }
        hit_res_t record = std::nullopt;
        constexpr float ray_nearest_t = 0.001F;
        if constexpr (std::is_same_v<T, bvh>) {
            record = target.hit(r, ray_nearest_t, g_max);
        } else if constexpr (std::is_same_v<T, world_t>) {
            record = hit(target, r, ray_nearest_t, g_max);
        } else {
            std::cerr << "unknown target type\n";
            std::abort();
        }
        if (!record.has_value()) {
            const auto direction = r.direction();
            const float elev = acos(direction.y());
            const float azim = atan2(direction.z(), direction.x());
            const float u = azim / (2 * g_pi) + 0.5;
            const float v = elev / g_pi;
            return envlight_.sample(u, v);
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

    /**
     * @brief 采样图片中的单个像素、渲染并写入颜色值
     *
     * @tparam T 场景聚合体的类型, 必须是: world_t 或 bvh
     * @param target 场景聚合体
     * @param row 像素所在行, 从上到下
     * @param col 像素所在列, 从左到右
     */
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
        color /= (float)config_.samples_per_pixel;
        write_color(row, col, color);
    }

    /**
     * @brief 通过模板统一是否使用BVH的两种情形; 内部条件判断统一是否使用多核算法
     *
     * @tparam T 场景聚合体的类型, 必须是: world_t 或 bvh
     * @param target 场景聚合体
     * @param path 图片文件的保存路径
     */
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
    /**
     * @brief 渲染场景, 将结果保存到图片文件中
     *
     * @param world 场景定义, 由一系列基本元素组成
     * @param path 图片文件的保存路径
     */
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
