#include "common.h"
#include "material.h"
#include "viewport.h"
#include "ray.h"
#include "sphere.h"
#include "utils.h"

#include <QDebug>
#include <fstream>

color_t ray_color(const ray &r, const world_t &world, int depth) {
    if (depth <= 0) {
        return {0, 0, 0};
    }
    auto record = hit(world, r, 0.001, g_infinity);
    if (record.has_value()) {
        ray scattered;
        color_t attenuation;
        if (record->pmat->scatter(r, record.value(), attenuation, scattered)) {
            return attenuation * ray_color(scattered, world, depth - 1);
        }
        return color_t{0, 0, 0};
    }
    float t = (r.direction().y() + 1.0f) / 2.0f;
    return (1.0f - t) * color_t{1.0, 1.0, 1.0} + t * color_t{0.5, 0.7, 1.0};
}

int main(int argc, char *argv[]) {
    constexpr float aspect_ratio = 16.0 / 9.0;
    constexpr int width = 400;
    constexpr int height = width / aspect_ratio;

    const viewport canvas{2.0f * aspect_ratio, 2.0f, 1.0f};
    constexpr point_t origin{0, 0, 0};

    auto material_ground = std::make_shared<lambertian>(color_t{0.8, 0.8, 0.0});
    auto material_center = std::make_shared<lambertian>(color_t{0.1, 0.2, 0.5});
    auto material_left   = std::make_shared<dielectric>(1.5);
    auto material_right  = std::make_shared<metal>(color_t{0.8, 0.6, 0.2}, 0.0);

    world_t world;
    world.push_back(std::make_shared<sphere>(point_t{0, -100.5, -1}, 100, material_ground));
    world.push_back(std::make_shared<sphere>(point_t{0, 0, -1},      0.5, material_center));
    world.push_back(std::make_shared<sphere>(point_t{-1, 0, -1},     0.5, material_left));
    world.push_back(std::make_shared<sphere>(point_t{-1, 0, -1},    -0.4, material_left));
    world.push_back(std::make_shared<sphere>(point_t{1, 0, -1},      0.5, material_right));

    std::string path = "../images/" + current_time() + ".ppm";
    std::ofstream file(path);
    file << "P3\n" << width << " " << height << "\n255\n";

    auto start = std::chrono::steady_clock::now();
    constexpr int samples_per_pixel = 100;
    constexpr int max_depth = 50;
    for (int j = height - 1; j >= 0; j--) {
        qDebug() << "\r scan lines remaining: " << j << ' ';
        for (int i = 0; i < width; i++) {
            color_t color;
            for (int s = 0; s < samples_per_pixel; s++) {
                const float u = ((float)i + random_float()) / (width - 1);
                const float v = ((float)j + random_float()) / (height - 1);
                ray r{origin, canvas.point_at(u, v)};
                color += ray_color(r, world, max_depth);
            }
            write_color(file, color, samples_per_pixel);
        }
    }
    auto end = std::chrono::steady_clock::now();

    qDebug() << "\n done in " << std::chrono::duration<double>(end - start).count() << "s.";
}
