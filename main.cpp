#include "common.h"
#include "camera.h"
#include "material.h"
#include "sphere.h"
#include "tracer.h"
#include "utils.h"

world_t random_world() {
    auto material_ground = std::make_shared<lambertian>(color_t{0.8, 0.8, 0.0});

    world_t world;
    world.push_back(std::make_shared<sphere>(point_t{0, -1000, 0}, 1000, material_ground));
    for (int a = -11; a < 11; a++) {
        for (int b = -11; b < 11; b++) {
            auto choose_mat = random_float();
            point_t center(a + 0.9f * random_float(), 0.2, b + 0.9f * random_float());

            if ((center - point_t(4, 0.2, 0)).len() > 0.9) {
                std::shared_ptr<material> sphere_material;

                if (choose_mat < 0.8) {
                    // diffuse
                    vec3_t albedo = {random_float(), random_float(), random_float()};
                    sphere_material = std::make_shared<lambertian>(albedo);
                    world.push_back(std::make_shared<sphere>(center, 0.2, sphere_material));
                } else if (choose_mat < 0.95) {
                    // metal
                    vec3_t albedo = {random_float(0.5, 1), random_float(0.5, 1), random_float(0.5, 1)};
                    auto fuzz = random_float(0, 0.5);
                    sphere_material = std::make_shared<metal>(albedo, fuzz);
                    world.push_back(std::make_shared<sphere>(center, 0.2, sphere_material));
                } else {
                    // glass
                    sphere_material = std::make_shared<dielectric>(1.5);
                    world.push_back(std::make_shared<sphere>(center, 0.2, sphere_material));
                }
            }
        }
    }

    auto material1 = std::make_shared<dielectric>(1.5);
    world.push_back(std::make_shared<sphere>(point_t(0, 1, 0), 1.0, material1));

    auto material2 = std::make_shared<lambertian>(vec3_t{0.4, 0.2, 0.1});
    world.push_back(std::make_shared<sphere>(point_t(-4, 1, 0), 1.0, material2));

    auto material3 = std::make_shared<metal>(vec3_t(0.7, 0.6, 0.5), 0.0);
    world.push_back(std::make_shared<sphere>(point_t(4, 1, 0), 1.0, material3));

    return world;
}

world_t simple_world() {
    world_t world;
    auto material_ground = std::make_shared<lambertian>(color_t{0.2, 0.8, 0.2});
    world.push_back(std::make_shared<sphere>(point_t{0, -100.5, 0}, 100, material_ground));
    auto mat1 = std::make_shared<dielectric>(1.5);
    auto mat2 = std::make_shared<lambertian>(vec3_t{1.0, 0.4, 0.4});
    auto mat3 = std::make_shared<metal>(vec3_t{0.2, 0.5, 0.8}, 0.1);
    world.push_back(std::make_shared<sphere>(point_t(-1, 0, -1), 0.5, mat1));
    world.push_back(std::make_shared<sphere>(point_t(-1, 0, -1), -0.45, mat1));
    world.push_back(std::make_shared<sphere>(point_t(0, 0, -1), 0.5, mat2));
    world.push_back(std::make_shared<sphere>(point_t(1, 0, -1), 0.5, mat3));
    return world;
}

int main(int argc, char *argv[]) {
    constexpr float aspect_ratio = 3.0 / 2.0;
    constexpr int width = 1200;
    constexpr int height = width / aspect_ratio;


    const auto world = random_world();
    const point_t lookfrom{13, 2, 3};
    const point_t lookat{0, 0, 0};
    const vec3_t vup{0, 1, 0};
    const float dist_to_focus = 10.0;
    const float aperture = 0.1;
    camera cam(lookfrom, lookat, vup, g_pi / 9.0, aspect_ratio, aperture, dist_to_focus);


/*
    const auto world = simple_world();
    const point_t eye{3, 3, 2};
    const point_t center{0, 0, -1};
    const vec3_t up{0, 1, 0};
    const float dist_to_focus = (eye - center).len();
    const float aperture = 2.0f;
    camera cam(eye, center, up, g_pi / 9.0, aspect_ratio, aperture, dist_to_focus);
*/

    constexpr int samples_per_pixel = 500;
    constexpr int max_depth = 50;
    tracer::config config{
            width, height,
            samples_per_pixel,
            max_depth, true
    };
    tracer my_tracer{config, cam};

    std::string path = "../images/" + current_time() + ".png";
    my_tracer.trace_png_parallel(world, path);
}
