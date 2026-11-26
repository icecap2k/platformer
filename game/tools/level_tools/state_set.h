#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

namespace level_tools {

// The set of situations the search has already seen: 64-bit keys in one flat array with open
// addressing (a colliding key takes the next free slot). A std::unordered_set allocates a node
// per element and costs around 40 bytes each; this one costs 8 bytes per slot and keeps its keys
// next to each other in memory, which is what the search spends most of its time on.
class StateSet {
public:
    StateSet() : slots_(kFirstSlots, kEmpty) {}

    // Adds the key. Returns true if it was not there yet.
    bool insert(std::uint64_t key) {
        if (key == kEmpty) {  // the value that marks a free slot is remembered aside
            const bool fresh = !has_empty_key_;
            has_empty_key_ = true;
            size_ += fresh ? 1 : 0;
            return fresh;
        }
        if ((size_ + 1) * kFullDen > slots_.size() * kFullNum) {
            grow();
        }
        if (!place(slots_, key)) {
            return false;
        }
        ++size_;
        return true;
    }

    std::size_t size() const { return size_; }

private:
    static constexpr std::uint64_t kEmpty = ~std::uint64_t{0};
    static constexpr std::size_t kFirstSlots = 1U << 10;  // always a power of two
    static constexpr std::size_t kFullNum = 4;            // grows beyond 4/5 full
    static constexpr std::size_t kFullDen = 5;

    // Spreads similar keys (neighbouring positions) all over the table (splitmix64 finaliser).
    static std::uint64_t mix(std::uint64_t value) {
        value = (value ^ (value >> 30)) * 0xBF58476D1CE4E5B9ULL;
        value = (value ^ (value >> 27)) * 0x94D049BB133111EBULL;
        return value ^ (value >> 31);
    }

    static bool place(std::vector<std::uint64_t>& slots, std::uint64_t key) {
        const std::size_t mask = slots.size() - 1;
        for (std::size_t at = static_cast<std::size_t>(mix(key)) & mask;; at = (at + 1) & mask) {
            if (slots[at] == key) {
                return false;
            }
            if (slots[at] == kEmpty) {
                slots[at] = key;
                return true;
            }
        }
    }

    void grow() {
        std::vector<std::uint64_t> bigger(slots_.size() * 2, kEmpty);
        for (const std::uint64_t key : slots_) {
            if (key != kEmpty) {
                place(bigger, key);
            }
        }
        slots_.swap(bigger);
    }

    std::vector<std::uint64_t> slots_;
    std::size_t size_ = 0;
    bool has_empty_key_ = false;
};

}  // namespace level_tools
