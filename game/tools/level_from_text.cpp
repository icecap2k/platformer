// level_from_text: turns a level drawn as text into a Tiled map the game loads.
//
//   level_from_text <input.txt> <output.tmj>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

#include "level_tools/text_level.h"

int main(int argc, char** argv) {
    if (argc != 3) {
        std::cerr << "usage: level_from_text <input.txt> <output.tmj>\n";
        return 2;
    }

    try {
        std::ifstream input(argv[1]);
        if (!input) {
            std::cerr << "error: cannot open " << argv[1] << '\n';
            return 2;
        }
        std::ostringstream contents;
        contents << input.rdbuf();

        const level_tools::TextLevel level = level_tools::parseTextLevel(contents.str());

        std::ofstream output(argv[2]);
        if (!output) {
            std::cerr << "error: cannot write " << argv[2] << '\n';
            return 2;
        }
        output << level_tools::toTiledJson(level);

        std::cout << "wrote " << argv[2] << " (" << level.width << "x" << level.height
                  << " tiles, " << level.coins.size() << " coins, " << level.neighbors.size()
                  << " neighbours)\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "error in " << argv[1] << ": " << error.what() << '\n';
        return 2;
    }
}
