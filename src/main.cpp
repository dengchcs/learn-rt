#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION

#include "parser.hpp"
#include "tracer.hpp"
#include "utils.hpp"

int main(int argc, char *argv[]) {
    if (argc != 3) {
        std::cerr << "usage: rt <scene-file> <image-dir>\n";
        return 1;
    }
    std::cout << "program started...\n\n";
    auto start = std::chrono::steady_clock::now();
    parser my_parser{argv[1]};
    auto world = my_parser.make_scene();
    auto my_tracer = my_parser.make_tracer();

    std::filesystem::path image_path(argv[2]);
    exist_or_abort(image_path, "image directory");
    image_path /= (current_time() + ".png");

    my_tracer.trace(world, image_path.string());
    auto end = std::chrono::steady_clock::now();
    std::cout << "done in " << std::chrono::duration<double>(end - start).count() << "s.\n";
    return 0;
}
