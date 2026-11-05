// Headless integration test; draw calls and key state are intercepted below.
// macOS:
// c++ -std=c++17 -O1 -g -fsanitize=address,undefined -Ivendor/raylib/include tests/inventory_test.cc game/inventory.cpp game/item.cpp physics/item_body.cpp physics/physics.cpp util.cpp vendor/raylib/lib/libraylib.a -framework OpenGL -framework Cocoa -framework IOKit -framework CoreVideo -o /tmp/inventory-test
#include "../game/item.h"
#include "../draw/draw.h"
#include "../physics/physics.h"

#include <array>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <set>

namespace {
std::array<bool, 512> keys{}, previous{};
std::vector<const meshedObject*> visible, shadows;
void require(bool ok, const char* message) {
    if (!ok) { std::fprintf(stderr, "FAIL: %s\n", message); std::exit(1); }
}
float distance(vector3 a, vector3 b) { vector3 d=a-b; return sqrtf(dot3(d,d)); }
void tap(player& p, int key) {
    keys[key]=true; handleItemInput(p);
    keys[key]=false; handleItemInput(p);
}
struct fixture {
    meshedObject object{};
    planeMtx original{}, transformed{};
    item it;
    void spawn(int index, player& p) {
        const vector3 v[]={{-.13f,-1.9f,-.13f},{.13f,1.74f,.13f},{-.13f,-1.9f,-.13f},{.13f,1.74f,.13f}};
        for(int i=0;i<4;++i) { original.m[i][0]=v[i].x; original.m[i][1]=v[i].y; original.m[i][2]=v[i].z; }
        object.colliderO=&original; object.collider=&transformed; object.cPlaneCount=1;
        object.pEntity.location={100.f+index*10,2,0};
        it=createItem(&object,8,60);
        require(addItem(&it),"spawn must register UUID");
        updateColliderLocation(object,p,false);
    }
};
void aimAt(player& p, item* target) {
    // Keep only the intended target within reach. No window is needed for range
    // and state transitions; detailed screen-space targeting stays in canInteract.
    int i=0;
    for(item* it:registeredItems()) if(it->state==itemState::ground) {
        it->obj->pEntity.location={100.f+10*i++,2,0};
        if(it==target) it->obj->pEntity.location={3,2,0};
        it->prevLoc=it->obj->pEntity.location;
        updateColliderLocation(*it->obj,p,false);
    }
    p.camera.camTarget={0,0,0};
}
} // namespace

bool getKeyPressedOnce(int key) {
    bool pressed=keys[key] && !previous[key]; previous[key]=keys[key]; return pressed;
}
void Draw3DGPU(const meshedObject& o,const camera&,shaderStore&,vector4,const mtx44*,const TextureCubemap*,bool) {
    visible.push_back(&o);
}
void Draw3DDepthGPU(const meshedObject& o,Shader,int) { shadows.push_back(&o); }

int main() {
    player p{}, other{};
    p.camera.camPos={0,2,0}; p.camera.up={0,1,0}; p.camera.fov=1.57079632679f;
    std::array<fixture,8> objects;
    for(int i=0;i<8;++i) objects[i].spawn(i,p);
    std::set<std::string> ids;
    for(const fixture& f:objects) {
        require(f.it.state==itemState::ground && !f.it.owner,"new items must be on ground");
        require((f.it.uuid.high&0xf000)==0x4000 && (f.it.uuid.low>>62)==2,"spawn ID must be a UUID v4");
        std::string id=itemUuidString(f.it.uuid);
        require(id.size()==36 && id[8]=='-' && id[13]=='-' && id[18]=='-' && id[23]=='-',"UUID must format canonically");
        require(ids.insert(id).second,"spawn UUIDs must be distinct");
    }
    require(!addItem(&objects[0].it),"same item cannot register twice");
    item duplicate=objects[0].it;
    require(!addItem(&duplicate),"different object cannot register a duplicate UUID");

    // Number keys choose the destination slot, not the first free slot.
    for(int i=0;i<6;++i) {
        tap(p,KEY_ONE+i);
        require(p.inventory.selectedSlot==i,"number keys 1-6 must select matching slot");
        aimAt(p,&objects[i].it); tap(p,KEY_F);
        require(p.inventory.slots[i]==objects[i].it.uuid,"pickup must assign UUID to selected slot");
        require(objects[i].it.state==itemState::player && objects[i].it.owner==&p,"pickup must establish ownership");
        require(heldItemObject(p)==&objects[i].object,"selected item must be equipped");
    }
    require(!selectInventorySlot(p,0) && !selectInventorySlot(p,7),"out-of-range slots must be rejected");
    require(!pickUpItem(other,objects[0].it.uuid,{},{},{1,0,0}),"another player cannot take stored items");

    // Stowed and equipped items do not fall. Only selected inventory geometry/shadows render.
    std::array<vector3,6> positions;
    for(int i=0;i<6;++i) positions[i]=objects[i].object.pEntity.location;
    world empty{nullptr,0};
    updateItemsPhysics(p,1.f/60,empty);
    for(int i=0;i<6;++i)
        require(distance(positions[i],objects[i].object.pEntity.location)==0,"all owned items must skip physics");
    shaderStore shader{};
    drawItems(p,shader,1,nullptr,nullptr,false); drawItemsDepth(p,{},0,1);
    require(visible.size()==3 && shadows.size()==3,"draw only two ground items and equipped inventory item");
    require(visible[0]==&objects[5].object && shadows[0]==&objects[5].object,"stowed items must not render or cast shadows");

    // A full inventory still swaps only the selected slot, dropping its contents at the current pose.
    tap(p,KEY_THREE);
    itemUuid original=objects[2].it.uuid, replacement=objects[6].it.uuid;
    p.camera.camPos={10,4,20}; p.pEntity.location={10,2,20};
    aimAt(p,nullptr);
    objects[6].object.pEntity.location={13,4,20};
    updateColliderLocation(objects[6].object,p,false);
    tap(p,KEY_F);
    require(p.inventory.slots[2]==replacement && selectedItem(p)==&objects[6].it,"occupied selected slot must swap");
    require(objects[2].it.uuid==original && objects[2].it.state==itemState::ground && !objects[2].it.owner,"displaced item must retain UUID and become ground");
    require(distance(objects[2].object.pEntity.location,p.camera.camPos)<4,"swapped item must drop near current player, not old stowed pose");
    require(distance(objects[2].object.pEntity.magnitude,{.25f,0,0})<1e-6f,"swap release must keep 0.25 forward velocity");
    for(int i=0;i<6;++i) if(i!=2)
        require(p.inventory.slots[i]==objects[i].it.uuid,"swap must preserve other five slots");

    // Invalid/unavailable pickup cannot drop the current item.
    require(!pickUpItem(p,{}, {}, {}, {1,0,0}),"nil ID pickup must fail");
    require(p.inventory.slots[2]==replacement,"failed pickup must preserve selected item");

    // F with no target drops once; holding F cannot instantly pick it back up.
    aimAt(p,nullptr);
    keys[KEY_F]=true; handleItemInput(p);
    require(p.inventory.slots[2].empty() && !heldItemObject(p),"F with no target must clear selected slot");
    for(int i=0;i<5;++i) handleItemInput(p);
    require(p.inventory.slots[2].empty(),"held F must not retrigger pickup");
    keys[KEY_F]=false; handleItemInput(p);
    require(objects[6].it.state==itemState::ground,"dropped item state must be ground");
    vector3 before=objects[6].object.pEntity.location;
    updateItemsPhysics(p,1.f/60,empty);
    require(objects[6].object.pEntity.location.y<before.y,"dropped item must resume gravity");

    // Switching to an empty slot equips nothing, and stored UUIDs remain unchanged.
    tap(p,KEY_TWO); require(heldItemObject(p)==&objects[1].object,"switching must equip stored item");
    tap(p,KEY_THREE); require(!heldItemObject(p),"empty slot must unequip without dropping other items");
    require(objects[1].it.state==itemState::player,"switching must leave previous item stored");

    // Blocked actions still consume edges, avoiding accidental drops when a menu closes.
    tap(p,KEY_ONE);
    keys[KEY_F]=true; handleItemInput(p,false); handleItemInput(p,true);
    require(selectedItem(p)==&objects[0].it,"F held through a menu must not drop on close");
    keys[KEY_F]=false; handleItemInput(p);
    keys[KEY_SIX]=true; handleItemInput(p,false); handleItemInput(p,true);
    require(p.inventory.selectedSlot==0,"slot key held through menu must not select on close");
    keys[KEY_SIX]=false; handleItemInput(p);

    // Shared wand tip must use the selected item's current world pose, not camera offset.
    for(vector3 rot : {vector3{0,0,0},vector3{.4f,1.2f,.3f},vector3{-.8f,-1.5707963f,0}}) {
        objects[0].object.pEntity.location={7,8,9}; objects[0].object.pEntity.rot=rot;
        vector3 top=objects[0].it.body.localCenter+vector3{0,objects[0].it.body.halfSize.y,0};
        vector4 expected=modmmult(buildModelMatrix({7,8,9},rot,{}),{top.x,top.y,top.z,1});
        require(distance(heldItemTip(p),{expected.x,expected.y,expected.z})<1e-5f,"spell origin must follow wand-tip transform");
    }

    // Two players have independent selection and UUID slots.
    require(selectInventorySlot(other,6),"other player can select slot six");
    require(pickUpItem(other,objects[7].it.uuid,{3,4,5},{},{1,0,0}),"other player can acquire a ground item");
    require(other.inventory.slots[5]==objects[7].it.uuid && p.inventory.slots[5]==objects[5].it.uuid,"player inventories must be independent");
    clearItems();
    require(registeredItems().empty(),"cleanup must clear registry");
    for(itemUuid id:p.inventory.slots) require(id.empty(),"cleanup must clear first player's UUID slots");
    for(itemUuid id:other.inventory.slots) require(id.empty(),"cleanup must clear other player's UUID slots");
    require(!findItem(original),"cleanup must clear UUID lookup");
    std::puts("inventory: UUIDs, six slots, input, swaps, ownership, physics, rendering, and tip origin passed");
}
