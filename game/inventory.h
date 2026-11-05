#pragma once

#include <array>
#include <cstdint>
#include <string>

// A nil UUID denotes an empty slot. IDs are assigned once at spawn and never
// change when picking up, stowing, swapping, or dropping an item.
struct itemUuid {
    std::uint64_t high = 0;
    std::uint64_t low = 0;
    bool empty() const { return high == 0 && low == 0; }
    bool operator==(itemUuid other) const { return high == other.high && low == other.low; }
    bool operator!=(itemUuid other) const { return !(*this == other); }
};

itemUuid generateItemUuid();
std::string itemUuidString(itemUuid id);

struct playerInventory {
    static constexpr int SLOT_COUNT = 6;
    std::array<itemUuid, SLOT_COUNT> slots{};
    int selectedSlot = 0; // zero-based internally; number keys/display use 1–6
};
