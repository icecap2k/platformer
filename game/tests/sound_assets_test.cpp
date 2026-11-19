#include <doctest/doctest.h>

#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>

#include "presentation/sound_cues.h"

namespace {

struct Wav {
    bool valid = false;
    int channels = 0;
    int sample_rate = 0;
    int bits = 0;
    int format = 0;
    std::vector<std::int16_t> samples;
};

std::uint32_t read32(const std::vector<unsigned char>& bytes, std::size_t at) {
    return static_cast<std::uint32_t>(bytes[at]) | static_cast<std::uint32_t>(bytes[at + 1]) << 8 |
           static_cast<std::uint32_t>(bytes[at + 2]) << 16 | static_cast<std::uint32_t>(bytes[at + 3]) << 24;
}

std::uint16_t read16(const std::vector<unsigned char>& bytes, std::size_t at) {
    return static_cast<std::uint16_t>(bytes[at] | bytes[at + 1] << 8);
}

// A tiny RIFF/WAVE reader: walks the chunks and reads "fmt " and "data".
Wav readWav(const std::string& path) {
    Wav wav;
    std::ifstream file(path, std::ios::binary);
    const std::vector<unsigned char> bytes((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    if (bytes.size() < 12 || std::string(bytes.begin(), bytes.begin() + 4) != "RIFF" ||
        std::string(bytes.begin() + 8, bytes.begin() + 12) != "WAVE") {
        return wav;
    }
    std::size_t at = 12;
    bool have_format = false;
    while (at + 8 <= bytes.size()) {
        const std::string id(bytes.begin() + static_cast<std::ptrdiff_t>(at),
                             bytes.begin() + static_cast<std::ptrdiff_t>(at) + 4);
        const std::size_t size = read32(bytes, at + 4);
        const std::size_t body = at + 8;
        if (body + size > bytes.size()) {
            return wav;  // truncated
        }
        if (id == "fmt " && size >= 16) {
            wav.format = read16(bytes, body);
            wav.channels = read16(bytes, body + 2);
            wav.sample_rate = static_cast<int>(read32(bytes, body + 4));
            wav.bits = read16(bytes, body + 14);
            have_format = true;
        } else if (id == "data") {
            for (std::size_t i = 0; i + 1 < size; i += 2) {
                wav.samples.push_back(static_cast<std::int16_t>(read16(bytes, body + i)));
            }
            wav.valid = have_format;
        }
        at = body + size + (size % 2);
    }
    return wav;
}

}  // namespace

TEST_CASE("every sound of the game has a valid, audible file") {
    for (const presentation::Sound sound : presentation::allSounds()) {
        const std::string name = presentation::soundName(sound);
        INFO("sound " << name);
        const Wav wav = readWav(std::string(PLATFORMER_ASSETS_DIR) + "/sounds/" + name + ".wav");

        REQUIRE(wav.valid);
        CHECK(wav.format == 1);          // PCM
        CHECK(wav.channels == 1);        // mono
        CHECK(wav.bits == 16);
        CHECK(wav.sample_rate == 22050);

        const double seconds = static_cast<double>(wav.samples.size()) / wav.sample_rate;
        CHECK(seconds >= 0.05);          // not a click...
        CHECK(seconds <= 1.5);           // ...and not a song: these are effects

        int peak = 0;
        double energy = 0.0;
        for (const std::int16_t sample : wav.samples) {
            peak = std::max(peak, std::abs(static_cast<int>(sample)));
            energy += static_cast<double>(sample) * sample;
        }
        CHECK(peak > 8000);              // audible
        CHECK(peak < 32767);             // normalised below full scale: it does not clip
        const double rms = std::sqrt(energy / static_cast<double>(wav.samples.size()));
        CHECK(rms > 1500.0);             // not just one spike in silence

        // It ends quietly (the fade-out): no click at the end.
        const std::size_t tail = wav.samples.size() - 1;
        CHECK(std::abs(static_cast<int>(wav.samples[tail])) < 800);
    }
}
