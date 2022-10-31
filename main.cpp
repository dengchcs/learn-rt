#include "parser.hpp"
#include "utils.hpp"

int main(int argc, char *argv[]) {
    if (argc != 2) {
        std::cerr << "usage: rt <scene-file>\n";
        return 1;
    }
    auto world = make_scene(argv[1]);
    auto my_tracer = make_tracer(argv[1]);
    std::string path = "../images/" + current_time() + ".png";
    my_tracer.trace(world, path);
    return 0;
}
