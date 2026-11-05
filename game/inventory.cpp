#include "item.h"

#include <cstdio>
#include <random>
#include <unordered_map>

namespace {
struct uuidHash {
    std::size_t operator()(itemUuid id) const {
        return std::hash<std::uint64_t>{}(id.high ^ (id.low + 0x9e3779b97f4a7c15ULL + (id.high<<6) + (id.high>>2)));
    }
};
std::vector<item*> gItems;
std::unordered_map<itemUuid, item*, uuidHash> gItemById;

bool validSlot(const player& p) {
    return p.inventory.selectedSlot >= 0 && p.inventory.selectedSlot < playerInventory::SLOT_COUNT;
}
} // namespace

itemUuid generateItemUuid() {
    // Random work happens only on spawn. Four random words form a UUID v4;
    // inventory lookups thereafter use two integers, not strings or mesh names.
    static std::random_device random;
    auto word = [&]() { return static_cast<std::uint64_t>(random()) & 0xffffffffULL; };
    itemUuid id{(word()<<32) | word(), (word()<<32) | word()};
    id.high = (id.high & ~0xf000ULL) | 0x4000ULL; // version 4
    id.low = (id.low & 0x3fffffffffffffffULL) | 0x8000000000000000ULL; // RFC variant
    return id;
}

std::string itemUuidString(itemUuid id) {
    char text[37];
    std::snprintf(text, sizeof(text), "%08x-%04x-%04x-%04x-%012llx",
                  static_cast<unsigned>(id.high>>32), static_cast<unsigned>((id.high>>16)&0xffff),
                  static_cast<unsigned>(id.high&0xffff), static_cast<unsigned>(id.low>>48),
                  static_cast<unsigned long long>(id.low&0xffffffffffffULL));
    return text;
}

bool addItem(item* it) {
    if (!it || !it->obj || it->uuid.empty() || it->state != itemState::ground || it->owner) return false;
    if (!gItemById.emplace(it->uuid, it).second) return false;
    gItems.push_back(it);
    return true;
}

const std::vector<item*>& registeredItems() { return gItems; }

item* findItem(itemUuid id) {
    auto found = gItemById.find(id);
    return found == gItemById.end() ? nullptr : found->second;
}

item* inventoryItem(const player& p, int slot) {
    if (slot < 1 || slot > playerInventory::SLOT_COUNT) return nullptr;
    item* it = findItem(p.inventory.slots[slot-1]);
    return it && it->state == itemState::player && it->owner == &p ? it : nullptr;
}

item* selectedItem(const player& p) {
    return validSlot(p) ? inventoryItem(p, p.inventory.selectedSlot+1) : nullptr;
}

meshedObject* heldItemObject(const player& p) {
    item* it = selectedItem(p);
    return it ? it->obj : nullptr;
}

bool selectInventorySlot(player& p, int slot) {
    if (slot < 1 || slot > playerInventory::SLOT_COUNT) return false;
    p.inventory.selectedSlot = slot-1;
    return true;
}

item* dropSelectedItem(player& p, vector3 location, vector3 rotation, vector3 direction) {
    if (!validSlot(p)) return nullptr;
    item* it = selectedItem(p);
    p.inventory.slots[p.inventory.selectedSlot] = {};
    if (!it) return nullptr;
    it->state = itemState::ground;
    it->owner = nullptr;
    it->obj->pEntity.location = location;
    it->obj->pEntity.rot = rotation;
    releaseItemBody(it->body, it->obj->pEntity, direction);
    it->prevLoc = location;
    return it;
}

bool pickUpItem(player& p, itemUuid id, vector3 heldLocation, vector3 heldRotation, vector3 dropDirection) {
    item* incoming = findItem(id);
    // Validate first: attempting to take an unavailable item must not discard
    // the selected item. Player-owned items cannot be taken from another slot/player.
    if (!validSlot(p) || !incoming || incoming->state != itemState::ground || incoming->owner) return false;
    dropSelectedItem(p, heldLocation, heldRotation, dropDirection);
    incoming->state = itemState::player;
    incoming->owner = &p;
    p.inventory.slots[p.inventory.selectedSlot] = incoming->uuid;
    incoming->obj->pEntity.location = heldLocation;
    incoming->obj->pEntity.rot = heldRotation;
    resetItemBody(incoming->body, incoming->obj->pEntity);
    incoming->prevLoc = heldLocation;
    return true;
}

void clearItems() {
    for (item* it : gItems) {
        if (it->owner) {
            for (itemUuid& slot : it->owner->inventory.slots)
                if (slot == it->uuid) slot = {};
        }
        it->owner = nullptr;
        it->state = itemState::ground;
    }
    gItemById.clear();
    gItems.clear();
}
