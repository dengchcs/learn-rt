#include "parser.hpp"
#include "utils.hpp"

int main(int argc, char *argv[]) {
    if (argc != 2) {
        std::cerr << "usage: rt <scene-file>\n";
        return 1;
    }
    std::cout << "program started...\n\n";
    auto start = std::chrono::steady_clock::now();
    auto world = make_scene(argv[1]);
    auto my_tracer = make_tracer(argv[1]);
    std::string path = "../images/" + current_time() + ".png";
    my_tracer.trace(world, path);
    auto end = std::chrono::steady_clock::now();
    std::cout << "done in " << std::chrono::duration<double>(end - start).count() << "s.\n";
    return 0;
}
