// Standalone, no window/GPU needed:
// c++ -std=c++17 -O2 -Ivendor/raylib/include tests/item_body_test.cc physics/item_body.cpp -o /tmp/item-body-test
#include "../physics/item_body.h"
#include "raymath.h"

#include <array>
#include <cmath>
#include <cstdio>
#include <cstdlib>

namespace {
void require(bool ok, const char* message) {
    if (!ok) { std::fprintf(stderr, "FAIL: %s\n", message); std::exit(1); }
}
float distance(vector3 a, vector3 b) {
    vector3 d = a-b; return sqrtf(d.x*d.x + d.y*d.y + d.z*d.z);
}
planeMtx quad(vector3 a, vector3 b, vector3 c, vector3 d) {
    planeMtx p{};
    vector3 v[] = {a,b,c,d};
    for (int i=0; i<4; ++i) { p.m[i][0]=v[i].x; p.m[i][1]=v[i].y; p.m[i][2]=v[i].z; }
    return p;
}
planeMtx floorQuad(float y, float left=-30.f, float right=30.f) {
    return quad({left,y,-30}, {right,y,-30}, {right,y,30}, {left,y,30});
}
struct fixture {
    meshedObject object{};
    itemBody body;
    planeMtx bounds;
    fixture(vector3 half, vector3 pos, vector3 rot={}, vector3 offset={}) {
        bounds = quad(offset-half, offset+half, offset-half, offset+half);
        object.colliderO=&bounds; object.cPlaneCount=1;
        object.pEntity.location=pos;
        object.pEntity.rot=rot;
        object.pEntity.weight=1;
        object.pEntity.acceleration={0,-20.5f,0};
        object.pEntity.applyAccel={1,1,1};
        initializeItemBody(body, object);
    }
    void tick(const world& w, float dt=1.f/60.f) { stepItemBody(body,object.pEntity,dt,w); }
    bool settle(const world& w, float seconds=15.f) {
        for (int i=0; i<(int)(seconds*60); ++i) {
            tick(w);
            require(std::isfinite(object.pEntity.location.y), "simulation must remain finite");
            if (body.sleeping) return true;
        }
        std::fprintf(stderr,"unsettled: p=(%.4f %.4f %.4f) v=(%.4f %.4f %.4f) w=(%.4f %.4f %.4f) quiet=%.3f\n",
                     object.pEntity.location.x,object.pEntity.location.y,object.pEntity.location.z,
                     object.pEntity.magnitude.x,object.pEntity.magnitude.y,object.pEntity.magnitude.z,
                     body.angularVelocity.x,body.angularVelocity.y,body.angularVelocity.z,body.quietTime);
        return false;
    }
    float bottom() const {
        Matrix m=QuaternionToMatrix(body.orientation);
        vector3 local=body.localCenter;
        float centerY=object.pEntity.location.y+m.m1*local.x+m.m5*local.y+m.m9*local.z;
        return centerY-fabsf(m.m1)*body.halfSize.x-fabsf(m.m5)*body.halfSize.y-fabsf(m.m9)*body.halfSize.z;
    }
};
}

int main() {
    planeMtx ground=floorQuad(0);
    worldSegment seg{&ground,1};
    world scene{&seg,1};
    world empty{nullptr,0};

    // Held-item Euler order, including the yaw singularity, must match rendering.
    for (float yaw : {-3.14f,-1.57079632679f,-.2f,1.57079632679f,2.4f}) {
        fixture rotation({.2f,.3f,.4f},{0,4,0},{.7f,yaw,-.3f});
        vector3 euler=itemBodyRenderRotation(rotation.body,1.f);
        Matrix a=QuaternionToMatrix(rotation.body.orientation);
        Matrix b=QuaternionToMatrix(QuaternionFromEuler(euler.x,euler.y,euler.z));
        require(fabsf(a.m0-b.m0)+fabsf(a.m5-b.m5)+fabsf(a.m9-b.m9)<.002f,"quaternion/Euler poses must agree at all camera yaws");
    }

    // Release direction and speed are independent of previous linear/angular motion.
    fixture released({.2f,1,.2f},{0,4,0},{.3f,.7f,.2f});
    for (vector3 d : {vector3{1,0,0},vector3{-1,2,3},vector3{0,-1,0}}) {
        released.body.angularVelocity={2,3,4}; released.body.sleeping=true;
        releaseItemBody(released.body,released.object.pEntity,d);
        float norm=sqrtf(d.x*d.x+d.y*d.y+d.z*d.z);
        require(distance(released.object.pEntity.magnitude,d.fmult(.25f/norm))<1e-6f,"release must be 0.25 along view");
        require(!released.body.sleeping && distance(released.body.angularVelocity,{})==0,"release must clear sleep/spin");
    }

    fixture flat({.5f,.25f,.35f},{0,4,0});
    require(flat.settle(scene),"flat box must settle and sleep");
    require(fabsf(flat.bottom())<.005f,"flat box must rest on floor");
    vector3 resting=flat.object.pEntity.location;
    Quaternion restRot=flat.body.orientation;
    for(int i=0;i<180;++i) flat.tick(scene);
    require(distance(resting,flat.object.pEntity.location)==0 && restRot.x==flat.body.orientation.x,"sleeping body must not drift");

    // An edge and a corner landing must create torque and reach a resting face.
    for (vector3 rot : {vector3{0,0,.55f},vector3{.45f,.2f,.55f}}) {
        fixture tipped({.25f,1.3f,.2f},{0,3,0},rot);
        releaseItemBody(tipped.body,tipped.object.pEntity,{1,0,0});
        bool spun=false;
        for(int i=0;i<240;++i) {
            tipped.tick(scene);
            spun |= distance(tipped.body.angularVelocity,{})>.2f;
        }
        require(spun,"edge/corner impact must cause tumbling");
        require(tipped.settle(scene),"tumbling box must settle");
        require(fabsf(tipped.bottom())<.005f,"tumbling box must not sink through floor");
        require(tipped.object.pEntity.location.y<.4f,"tall tilted box should fall onto a side");
    }

    for (vector3 rot : {vector3{0,0,.78539816f},vector3{-.78539816f,0,.61547971f}}) {
        fixture balanced({.4f,.4f,.4f},{0,2,0},rot);
        releaseItemBody(balanced.body,balanced.object.pEntity,{1,0,0});
        require(balanced.settle(scene),"precisely balanced cube must settle after forward release");
        require(fabsf(balanced.object.pEntity.location.y-.4f)<.01f,"edge/corner cube must end on a face");
    }

    // Skinny wand and quarter-scale item at the actual fixed timestep.
    for (float scale : {1.f,.25f}) {
        fixture wand({.13375f*scale,1.82255f*scale,.13375f*scale},{0,4,0},{.2f,.8f,.4f},{0,-.07907f*scale,0});
        releaseItemBody(wand.body,wand.object.pEntity,{.7f,.2f,.3f});
        require(wand.settle(scene),"wand-sized body must settle");
        require(fabsf(wand.bottom())<.005f,"wand-sized body must stay above the floor");
    }

    // The origin need not coincide with center of mass: free spin must not orbit it.
    fixture offcenter({.2f,1,.3f},{0,5,0},{},{0,2,0});
    offcenter.object.pEntity.acceleration={}; offcenter.body.angularVelocity={1,2,3};
    vector3 initialCenter={0,7,0};
    for(int i=0;i<120;++i) offcenter.tick(empty);
    Matrix m=QuaternionToMatrix(offcenter.body.orientation);
    vector3 center=offcenter.object.pEntity.location+vector3{m.m4*2,m.m5*2,m.m6*2};
    require(distance(center,initialCenter)<.001f,"rotation must preserve center of mass in free space");

    // Overhanging a finite ledge cannot sleep or balance on an infinite floor extension.
    std::array<planeMtx,2> ledges={floorQuad(0,-5,0),floorQuad(-4)};
    worldSegment ledgeSeg{ledges.data(),2}; world ledgeScene{&ledgeSeg,1};
    fixture overhang({1,.25f,.4f},{.7f,.25f,0});
    require(overhang.settle(ledgeScene),"overhanging item should tumble to the lower floor");
    require(overhang.object.pEntity.location.y < -2.5f,"overhang must fall off the ledge");

    // A shallow slope can support the body with friction.
    planeMtx slope=floorQuad(0);
    for(int i=0;i<4;++i) slope.m[i][1]=slope.m[i][0]*.25f;
    worldSegment slopeSeg{&slope,1}; world slopeScene{&slopeSeg,1};
    fixture onSlope({.5f,.25f,.3f},{0,3,0},{.1f,.4f,.2f});
    require(onSlope.settle(slopeScene),"body must settle on a shallow slope");

    // Wakes if the supporting world surface moves or disappears.
    ground=floorQuad(-2);
    flat.tick(scene);
    require(!flat.body.sleeping,"moving support must wake sleeping body");
    require(flat.settle(scene),"body must settle again on moved support");
    flat.tick(empty);
    require(!flat.body.sleeping,"removing support must wake sleeping body");

    // Moderate high-speed falls, plus a wall, exercise substeps and non-ground contacts.
    ground=floorQuad(0);
    fixture fast({.13f,.9f,.13f},{0,5,0},{.4f,.2f,.5f});
    fast.object.pEntity.magnitude={0,-40,0};
    require(fast.settle(scene),"fast thin body must not tunnel through floor");
    require(fabsf(fast.bottom())<.005f,"fast fall must end above ground");
    std::array<planeMtx,2> room={floorQuad(0),quad({1,0,-10},{1,10,-10},{1,10,10},{1,0,10})};
    worldSegment roomSeg{room.data(),2}; world roomScene{&roomSeg,1};
    fixture wall({.25f,.4f,.3f},{0,2,0},{.2f,.1f,.3f});
    wall.object.pEntity.magnitude={4,0,0};
    require(wall.settle(roomScene),"wall contact must eventually settle on floor");
    require(wall.object.pEntity.location.x<1.f,"body must not pass through wall");

    // Mesh collider imports may contain triangular faces with a repeated vertex.
    std::array<planeMtx,2> triangles={
        quad({-30,0,-30},{30,0,-30},{30,0,30},{30,0,30}),
        quad({-30,0,-30},{30,0,30},{-30,0,30},{-30,0,30})};
    worldSegment triSeg{triangles.data(),2}; world triScene{&triSeg,1};
    fixture triangleDrop({.2f,.8f,.2f},{0,3,0},{.4f,.3f,.2f});
    require(triangleDrop.settle(triScene),"body must settle across a triangular collider seam");
    require(fabsf(triangleDrop.bottom())<.005f,"triangle seam must not let item sink");

    // Multiple drop attitudes and sizes catch solver instability beyond hand-picked cases.
    for (int i=0;i<48;++i) {
        float scale=(i%3==0)?.25f:1.f;
        fixture randomized({.13375f*scale,1.82255f*scale,.13375f*scale},
                           {0,2.f+(i%4),0},{i*.371f,i*.613f,i*.197f});
        releaseItemBody(randomized.body,randomized.object.pEntity,{cosf(i*.4f),.3f,sinf(i*.4f)});
        require(randomized.settle(scene),"varied wand drops must settle");
        require(fabsf(randomized.bottom())<.005f,"varied wand drops must stay on floor");
    }

    std::puts("item body physics: all checks passed");
}
