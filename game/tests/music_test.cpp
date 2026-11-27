#include <doctest/doctest.h>

#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>

#include "presentation/music_volume.h"

using namespace presentation;

TEST_CASE("the music volume moves in steps and stays in range") {
    MusicVolume volume;
    CHECK(volume.step() == kDefaultMusicStep);
    volume.change(1);
    CHECK(volume.step() == kDefaultMusicStep + 1);
    volume.change(100);
    CHECK(volume.step() == kMusicSteps);
    CHECK(volume.gain(false) == 1.0F);
    volume.change(-100);
    CHECK(volume.step() == 0);
    CHECK(volume.gain(false) == 0.0F);
    CHECK(volume.label() == "MUSIC OFF");
    volume.change(3);
    CHECK(volume.label() == "MUSIC 3/10");
}

TEST_CASE("the music is quieter while paused, and each step is louder than the one below") {
    MusicVolume volume;
    CHECK(volume.gain(true) < volume.gain(false));
    volume.change(-100);
    float previous = volume.gain(false);
    for (int step = 1; step <= kMusicSteps; ++step) {
        volume.change(1);
        CHECK(volume.gain(false) > previous);
        previous = volume.gain(false);
    }
}

TEST_CASE("the cave music is a long, gentle loop without a seam") {
    std::ifstream file(std::string(PLATFORMER_ASSETS_DIR) + "/music/cave.wav", std::ios::binary);
    REQUIRE(file.good());
    const std::vector<unsigned char> bytes((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    REQUIRE(bytes.size() > 44);
    CHECK(std::string(bytes.begin(), bytes.begin() + 4) == "RIFF");

    // The generator writes a plain 44 byte header: 16-bit mono at 22050 Hz, then the samples.
    std::vector<std::int16_t> samples;
    for (std::size_t at = 44; at + 1 < bytes.size(); at += 2) {
        samples.push_back(static_cast<std::int16_t>(bytes[at] | bytes[at + 1] << 8));
    }
    const double seconds = static_cast<double>(samples.size()) / 22050.0;
    CHECK(seconds > 20.0);
    CHECK(seconds < 60.0);

    int peak = 0;
    double energy = 0.0;
    int biggest_step = 0;
    for (std::size_t i = 0; i < samples.size(); ++i) {
        peak = std::max(peak, std::abs(static_cast<int>(samples[i])));
        energy += static_cast<double>(samples[i]) * samples[i];
        if (i > 0) {
            biggest_step = std::max(biggest_step, std::abs(samples[i] - samples[i - 1]));
        }
    }
    CHECK(peak < 26000);  // background: well below full scale
    CHECK(std::sqrt(energy / static_cast<double>(samples.size())) > 1500.0);
    // Going from the last sample back to the first is no bigger a jump than the music makes anyway.
    CHECK(std::abs(samples.front() - samples.back()) <= biggest_step);
}
