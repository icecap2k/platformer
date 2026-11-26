// level_check: proves that every room and every coin of a world can be reached.
//
//   level_check [--full] <folder with .tmj files> [start_room] [spawn_x spawn_y]
//
// It stops as soon as everything has been found; --full explores every situation anyway.
//
// Exit code: 0 = everything reachable, 1 = something is not, 2 = bad input.
#include <algorithm>
#include <filesystem>
#include <iostream>
#include <string>
#include <vector>

#include "infrastructure/assets/tiled_map_loader.h"
#include "level_tools/reachability.h"

namespace {

void printUsage() {
    std::cerr << "usage: level_check [--full] <folder with .tmj files> [start_room] [spawn_x spawn_y]\n"
                 "  start_room defaults to room_01, spawn to 40 60 (the game's values)\n";
}

}  // namespace

int main(int argc, char** argv) {
    const bool full = argc > 1 && std::string(argv[1]) == "--full";
    if (full) {  // take the flag out, so the rest is read as without it
        ++argv;
        --argc;
    }
    if (argc != 2 && argc != 3 && argc != 5) {
        printUsage();
        return 2;
    }

    try {
        const std::filesystem::path folder = argv[1];
        std::vector<std::filesystem::path> files;
        for (const auto& entry : std::filesystem::directory_iterator(folder)) {
            if (entry.path().extension() == ".tmj") {
                files.push_back(entry.path());
            }
        }
        std::sort(files.begin(), files.end());
        if (files.empty()) {
            std::cerr << "no .tmj files in " << folder << '\n';
            return 2;
        }

        std::vector<domain::Room> rooms;
        for (const auto& file : files) {
            rooms.push_back(infrastructure::loadTiledRoom(file.string()));
        }
        std::size_t enemy_count = 0;
        for (const domain::Room& room : rooms) {
            enemy_count += room.enemies().size();
        }
        const domain::RoomGraph world(std::move(rooms));

        level_tools::ReachabilityOptions options;
        options.start_room = argc >= 3 ? argv[2] : "room_01";
        options.stop_when_all_found = !full;
        if (argc == 5) {
            options.spawn_x = std::stof(argv[3]);
            options.spawn_y = std::stof(argv[4]);
        }

        const level_tools::ReachabilityReport report = level_tools::analyzeReachability(world, options);

        std::cout << "Loaded " << world.size() << " rooms from " << folder.string() << '\n'
                  << "Start: " << options.start_room << " at (" << options.spawn_x << ", "
                  << options.spawn_y << ")\n"
                  << "Explored " << report.states_explored << " states"
                  << (report.truncated ? " (INCOMPLETE: search limit reached)" : "")
                  << (report.stopped_early ? " (everything found)" : "") << "\n\n";

        std::cout << "Rooms:\n";
        for (const auto& [id, reachable] : report.room_reachable) {
            std::cout << "  " << id << "  " << (reachable ? "reachable" : "UNREACHABLE")
                      << (report.room_standable.at(id) ? ", can stand" : "") << '\n';
        }

        std::cout << "\nCoins:\n";
        if (report.coins.empty()) {
            std::cout << "  (none)\n";
        }
        for (const level_tools::CoinStatus& coin : report.coins) {
            std::cout << "  " << coin.room << " #" << coin.index << " at (" << coin.coin.x << ", "
                      << coin.coin.y << ")  " << (coin.touchable ? "OK" : "CANNOT BE TOUCHED")
                      << '\n';
        }

        if (!report.fruits.empty()) {
            std::cout << "\nFruits:\n";
            for (const level_tools::FruitStatus& fruit : report.fruits) {
                std::cout << "  " << fruit.room << " #" << fruit.index << " at (" << fruit.fruit.x << ", "
                          << fruit.fruit.y << ")  " << (fruit.touchable ? "OK" : "CANNOT BE TOUCHED")
                          << '\n';
            }
        }

        if (!report.checkpoints.empty()) {
            std::cout << "\nCheckpoints:\n";
            for (const level_tools::CheckpointStatus& checkpoint : report.checkpoints) {
                std::cout << "  " << checkpoint.room << " #" << checkpoint.index << " at (" << checkpoint.checkpoint.x
                          << ", " << checkpoint.checkpoint.y << ")  "
                          << (checkpoint.touchable ? "OK" : "CANNOT BE TOUCHED") << '\n';
            }
        }

        if (!report.orbs.empty()) {
            std::cout << "\nOrbs (all of them are needed to open the exit):\n";
            for (const level_tools::OrbStatus& orb : report.orbs) {
                std::cout << "  " << orb.room << " #" << orb.index << " at (" << orb.orb.x << ", " << orb.orb.y
                          << ")  " << (orb.touchable ? "OK" : "CANNOT BE TOUCHED") << '\n';
            }
        }

        if (!report.exits.empty()) {
            std::cout << "\nExits:\n";
            for (const level_tools::ExitStatus& exit : report.exits) {
                std::cout << "  " << exit.room << " #" << exit.index << " at (" << exit.exit.x << ", " << exit.exit.y
                          << ")  " << (exit.touchable ? "OK" : "CANNOT BE TOUCHED") << '\n';
            }
        }

        // Enemies are not part of the search (ADR-0014): say so, so nobody trusts it for them.
        if (enemy_count > 0) {
            std::cout << "\nEnemies: " << enemy_count
                      << " placed; NOT modelled by this check (it assumes they never get in the way)\n";
        }

        if (report.allOk() && !report.truncated) {
            std::cout << "\nResult: OK\n";
            return 0;
        }
        std::cout << "\nResult: FAILED\n";
        return 1;
    } catch (const std::exception& error) {
        std::cerr << "error: " << error.what() << '\n';
        return 2;
    }
}
