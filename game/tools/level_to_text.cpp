// level_to_text: draws an existing Tiled map as text (the reverse of level_from_text).
//
//   level_to_text <input.tmj> <output.txt>
#include <fstream>
#include <iostream>

#include "infrastructure/assets/tiled_map_loader.h"
#include "level_tools/text_level.h"

int main(int argc, char** argv) {
    if (argc != 3) {
        std::cerr << "usage: level_to_text <input.tmj> <output.txt>\n";
        return 2;
    }

    try {
        const domain::Room room = infrastructure::loadTiledRoom(argv[1]);
        const std::string text = level_tools::toText(room);

        std::ofstream output(argv[2]);
        if (!output) {
            std::cerr << "error: cannot write " << argv[2] << '\n';
            return 2;
        }
        output << text;
        std::cout << "wrote " << argv[2] << '\n';
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "error in " << argv[1] << ": " << error.what() << '\n';
        return 2;
    }
}
