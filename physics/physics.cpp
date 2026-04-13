#include "../util.h"
#include <iostream>
#include <cmath>
#include <cstring>
#include "raylib.h"
#include "string"

/**
 * Analytical edge collision detection FROM -> Claude.ai
 * Segment-Quad Intersection:

    Given:
    A, B       — endpoints of the edge (world space)
    p1         — first vertex of the quad
    normal     — cross(p2-p1, p4-p1)

    Step 1 — find t where segment crosses the infinite plane:

    denom = dot(B - A, normal)
    if |denom| < eps → edge is parallel, skip

    t = dot(p1 - A, normal) / denom
    if t < 0 or t > 1 → crossing is outside segment, skip

    Step 2 — compute crossing point:

    P = A + t * (B - A)

    Step 3 — check P is inside the quad:

    u = p2 - p1
    v = p4 - p1
    w = P  - p1

    denom2  = dot(u,u)*dot(v,v) - dot(u,v)^2
    normedYD = (dot(w,u)*dot(v,v) - dot(w,v)*dot(u,v)) / denom2
    normedZD = (dot(w,v)*dot(u,u) - dot(w,u)*dot(u,v)) / denom2

    if normedYD in [0,1] and normedZD in [0,1] → COLLISION at point P

    Repositioning:
    t tells you how far along the edge the hit occurred.
    Push the object back along its velocity by (1-t) * |B-A|
    or reposition so P sits exactly on the quad surface.

 */
bool analyticalEdgeCollision(physicsEntity& player, planeMtx plane, vector3& applyAcc, planeMtx* collider, int collider_depth,float conservationPercent, float deltaTime, int& target, bool invertedNormals, bool& hasCollidedGround, bool& hasCollidedWall, int planeIndex) {
    vector3 p1 = {plane.m[0][0], plane.m[0][1], plane.m[0][2]};
    vector3 p2 = {plane.m[1][0], plane.m[1][1], plane.m[1][2]};
    vector3 p3 = {plane.m[2][0], plane.m[2][1], plane.m[2][2]};
    vector3 p4 = {plane.m[3][0], plane.m[3][1], plane.m[3][2]};
    
    vector3 v1 = p2 - p1;
    vector3 v2 = p4 - p1;

    vector3 normal = cross3(v1, v2);

    // new math for closest point on finite quad and NOT infinite plane ! very important

    float n2 = dot3(normal, normal);
    if (n2 < eps) return false;

    vector3 post_impact_vel = normal.fmult(dot3(player.magnitude, normal) / n2);
    
    // checking collision for each edge of each plane in the collider
    for (int i = 0; i < collider_depth; i++) {
        for (int j = 0; j < 4; j++) {
            planeMtx colPlane = collider[i];
            int next = (j + 1) % 4;
            vector3 A = {colPlane.m[j][0], colPlane.m[j][1], colPlane.m[j][2]};
            vector3 B = {colPlane.m[next][0], colPlane.m[next][1], colPlane.m[next][2]};

            float denom = dot3(B - A, normal);
            if (fabsf(denom) < eps) continue;

            float tplane = dot3(p1 - A, normal) / denom;
            if (tplane < 0.f || tplane > 1.f) continue;

            vector3 P = A + (B - A).fmult(tplane);
            vector3 u = p2 - p1;
            vector3 v = p4 - p1;
            vector3 w = P - p1;

            float denom2 = dot3(u,u)*dot3(v,v) - dot3(u,v)*dot3(u,v);
            if (fabsf(denom2) < eps) continue;
            float normedYD = (dot3(w,u)*dot3(v,v) - dot3(w,v)*dot3(u,v)) / denom2;
            float normedZD = (dot3(w,v)*dot3(u,u) - dot3(w,u)*dot3(u,v)) / denom2;

            if (normedYD >= 0.f && normedYD <= 1.f && normedZD >= 0.f && normedZD <= 1.f) {
                // collision with edge plane
                bool isGround = dot3(normalize3(normal), {0, 1, 0}) < -0.7f;
                if (isGround) {
                    if (!hasCollidedGround) {
                        float distA = dot3(A - p1, normal) / normal.mag();
                        float distB = dot3(B - p1, normal) / normal.mag();
                        float penetration = (distA < distB) ? distA : distB;
                        player.location = player.location + eps;
                        hasCollidedGround = true;
                        player.groundPlane = planeIndex;
                    }
                    player.collidingY = true;
                } else {
                    if (!hasCollidedWall) {
                        player.magnitude.x -= post_impact_vel.x * conservationPercent;
                        player.magnitude.z -= post_impact_vel.z * conservationPercent;
                        hasCollidedWall = true;
                    }
                }
                return true;
            }
        }
    }
    return false;
}

/**
 * Math from ChatGPT: https://chatgpt.com/share/69a0d1cd-2908-8001-a52c-c762d5f91148
 *
 *
 */
bool spherePlaneCollide(physicsEntity& player, planeMtx plane, vector3& applyAcc, float conservationPercent, float deltaTime, int& target, bool invertedNormals, bool& hasCollidedGround, bool& hasCollidedWall, int planeIndex) {
    vector3 p1 = {plane.m[0][0], plane.m[0][1], plane.m[0][2]};
    vector3 p2 = {plane.m[1][0], plane.m[1][1], plane.m[1][2]};
    vector3 p3 = {plane.m[2][0], plane.m[2][1], plane.m[2][2]};
    vector3 p4 = {plane.m[3][0], plane.m[3][1], plane.m[3][2]};


    vector3 v1 = p2 - p1;
    vector3 v2 = p4 - p1;

    vector3 normal = cross3(v1, v2);

    // new math for closest point on finite quad and NOT infinite plane ! very important

    float n2 = dot3(normal, normal);
    if (n2 < eps) return false;

    vector3 post_impact_vel = normal.fmult(dot3(player.magnitude, normal) / n2);

    float tplane = dot3(normal, player.location - p1) / n2;
    vector3 proj = player.location - normal.fmult(tplane);

    vector3 u = p2 - p1;
    vector3 v = p4 - p1;
    vector3 t = proj - p1;

    float dotu = dot3(u,u), dotuv = dot3(u,v), dotv = dot3(v,v);
    float dotwu = dot3(t,u), dotwv = dot3(t,v);
    float d_check = dotu*dotv - dotuv*dotuv;
    if (fabsf(d_check) < eps) return false;

    float normedYD = (dotwu * dotv - dotwv * dotuv) / d_check;
    float normedZD = (dotwv * dotu - dotwu * dotuv) / d_check;

    if (normedYD < 0.f || normedYD > 1.f) return false;
    if (normedZD < 0.f || normedZD > 1.f) return false;

    bool isGround = dot3(normalize3(normal), {0, 1, 0}) < -0.7; // inverted normals moment

    vector3 close_point = p1 + u.fmult(normedYD) + v.fmult(normedZD); // p1 coz its with respect to the plane
    float signedDist = dot3(player.location - close_point, normal) / normal.mag();
    // old but still works

    // std::cout << "here" << std::endl;
    // std::cout << distance << std::endl;

    if (signedDist > -0.2f && signedDist < 0.2f) {
//        std::cout << "COLLIDING" << std::endl;
        if (isGround) {
            if (!hasCollidedGround) {
                vector3 repos = close_point + normal.fmult(-0.20001f / normal.mag());
                if (repos.y > player.location.y) player.location.y = repos.y;
                hasCollidedGround = true;
                // std::cout << "standing on plane: " << planeIndex << std::endl;
                player.groundPlane = planeIndex;
            }
            player.collidingY = true;
        } else {
            if (!hasCollidedWall) {
                vector3 repos = close_point + normal.fmult(-0.20001f / normal.mag());
                player.location.x = repos.x;
                player.location.z = repos.z;
                hasCollidedWall = true;
            }
            player.magnitude.x -= post_impact_vel.x * (1) * conservationPercent;
            player.magnitude.z -= post_impact_vel.z * (1) * conservationPercent;
        }

        // handle non-physics related collision
        // (plane->action)(plane->id);
        // target = plane->id;

        // if (plane.id == 0) {

        //     return false;
        // }
        // else {
        //     return true;
        // }
        return true;
    }
    return false;
}

void applyForce(vector3 newForce, physicsEntity& pEntity) {
    pEntity.newForce = newForce;
}

void applyAcceleration(vector3 newAccel, physicsEntity& pEntity) {
    pEntity.acceleration = pEntity.acceleration + newAccel;
}

void processPhysics(float deltaTime, int frameRate, physicsEntity& pEntity, world& world, bool& end, int& target, bool invertedNormals, bool collide, planeMtx* collider, int collider_depth) {
    bool acc = false;
    bool hasCollidedGround = false;
    bool hasCollidedWall = false;

    // checking flat collision for each plane in the world (probably should dynamically build this)
    if (collide) {
        pEntity.collidingY = false;
        constexpr float CULL_DIST = 250.0f; // skip planes whose center is farther than this
        for (int wtc = 0; wtc < world.planeCount; wtc++) {
            // Early exit: both ground and wall already resolved
            if (hasCollidedGround && hasCollidedWall) break;

            // Distance cull: skip planes whose AABB center is far from the player
            const planeMtx& pl = world.planes[wtc];
            float cx = (pl.m[0][0] + pl.m[1][0] + pl.m[2][0] + pl.m[3][0]) * 0.25f;
            float cy = (pl.m[0][1] + pl.m[1][1] + pl.m[2][1] + pl.m[3][1]) * 0.25f;
            float cz = (pl.m[0][2] + pl.m[1][2] + pl.m[2][2] + pl.m[3][2]) * 0.25f;
            float dx = cx - pEntity.location.x;
            float dy = cy - pEntity.location.y;
            float dz = cz - pEntity.location.z;
            if (dx*dx + dy*dy + dz*dz > CULL_DIST * CULL_DIST) continue;

            if (pEntity.complexGeometry) {
                analyticalEdgeCollision(pEntity, world.planes[wtc], pEntity.applyAccel, collider, collider_depth, 1, deltaTime, target, invertedNormals, hasCollidedGround, hasCollidedWall, wtc);
            }
            else {
                spherePlaneCollide(pEntity, world.planes[wtc], pEntity.applyAccel, 1, deltaTime, target, invertedNormals, hasCollidedGround, hasCollidedWall, wtc);
            }
        }
    }
    // force transfer

    if (pEntity.newForce.x !=0 || pEntity.newForce.y !=0 || pEntity.newForce.z !=0 || acc == true) {
//        std::cout << pEntity.newForce.y << std::endl;
//        std::cout << pEntity.acceleration.y << std::endl;
        pEntity.acceleration = (pEntity.acceleration + pEntity.newForce).fdiv(pEntity.weight);

        pEntity.newForce.murder();
//        std::cout << pEntity.newForce.y << std::endl;
    }
    // acceleration
            // std::cout << "delta: " << deltaTime << std::endl;
//            std::cout << pEntity.applyAccel.y << std::endl;
    pEntity.magnitude = pEntity.magnitude + (pEntity.acceleration * pEntity.applyAccel).fmult(deltaTime);
    if (pEntity.collidingY && pEntity.magnitude.y < 0.f) pEntity.magnitude.y = 0.f;
//    std::cout << "player mag-y: " << pEntity.magnitude.y << std::endl;


    // lol no more terminal velocity...

//    // force terminal velocity
//    if (pEntity.magnitude.y > 214.f) {
//        pEntity.magnitude.y = 214.f; // roughly terminal velocity if a 3m sphere weighing 12500kg ;p
//    }
    pEntity.location.x += pEntity.magnitude.x * deltaTime;
    pEntity.location.y += pEntity.magnitude.y * deltaTime;
    pEntity.location.z += pEntity.magnitude.z * deltaTime;

}

/*
 vector3 magnitude; // should make this substruct
 vector3 newForce;
 vector3* accelForces;
 int maxAccelForces;
 int accelForcesCount;
 vector3 applyAccel;
 */
void initializePhysicsEntity(physicsEntity& pEntity, float weight, physicsComplexity complexity) {
    // assume location is set in object/player creation
    pEntity.acceleration = {0.f,0.f,0.f};
    pEntity.applyAccel = {1.f, 1.f, 1.f};
    pEntity.newForce = {0.f, 0.f, 0.f};
    pEntity.magnitude = {0.f, 0.f, 0.f};
    pEntity.weight = weight;
    pEntity.collidingY = false;
    pEntity.jumping = false;
    pEntity.groundPlane = -1;
    pEntity.velocity = 0.f;
    if (complexity == COMPLEX) {
        pEntity.complexGeometry = true;
    } else {
        pEntity.complexGeometry = false;
    }
}

//void updatePlayerLocation(player)

void updateEntityLocation(meshedObject& object) {
    float c = cosf(object.rotY);
    float s = sinf(object.rotY);
    for (int i = 0; i < object.mesh.count; i++) {
        for (int j = 0; j < 3; j++) {
            float ox = object.mesh.trisO[i].v[j].x;
            float oy = object.mesh.trisO[i].v[j].y;
            float oz = object.mesh.trisO[i].v[j].z;
            object.mesh.tris[i].v[j].x = (ox * c - oz * s) + object.pEntity.location.x;
            object.mesh.tris[i].v[j].y = oy + object.pEntity.location.y;
            object.mesh.tris[i].v[j].z = (ox * s + oz * c) + object.pEntity.location.z;

            float onx = object.mesh.trisO[i].n[j].x;
            float ony = object.mesh.trisO[i].n[j].y;
            float onz = object.mesh.trisO[i].n[j].z;
            object.mesh.tris[i].n[j].x = (onx * c - onz * s);
            object.mesh.tris[i].n[j].y = ony;
            object.mesh.tris[i].n[j].z = (onx * s + onz * c);
        }
    }
}

void updateColliderLocation(meshedObject& object) {
    for (int i = 0; i < object.cPlaneCount; i++) {
        for (int v = 0; v < 4; v++) {
            object.collider[i].m[v][0] = object.colliderO[i].m[v][0] + object.pEntity.location.x;
            object.collider[i].m[v][1] = object.colliderO[i].m[v][1] + object.pEntity.location.y;
            object.collider[i].m[v][2] = object.colliderO[i].m[v][2] + object.pEntity.location.z;
        }
    }
}

void resetMeshedLocation(meshedObject& object) {
    for (int i = 0; i < object.mesh.count; i++) {
        for (int j = 0; j < 3; j++) {
            object.mesh.tris[i].v[j].x = object.mesh.trisO[i].v[j].x;
            object.mesh.tris[i].v[j].y = object.mesh.trisO[i].v[j].y;
            object.mesh.tris[i].v[j].z = object.mesh.trisO[i].v[j].z;
        }
    }
    for (int i = 0; i < object.cPlaneCount; i++) {
        for (int v = 0; v < 4; v++) {
            object.collider[i].m[v][0] = object.colliderO[i].m[v][0];
            object.collider[i].m[v][1] = object.colliderO[i].m[v][1];
            object.collider[i].m[v][2] = object.colliderO[i].m[v][2];
        }
    }
}


world buildWorld(meshedObject** objects, int objectCount) {
    static planeMtx* planes = nullptr;
    static int allocatedCount = 0;

    int totalPlanes = 0;
    for (int i = 0; i < objectCount; i++) {
        updateColliderLocation(*objects[i]);
        totalPlanes += objects[i]->cPlaneCount;
    }

    if (totalPlanes > allocatedCount) {
        delete[] planes;
        planes = new planeMtx[totalPlanes];
        allocatedCount = totalPlanes;
    }

    int offset = 0;
    for (int i = 0; i < objectCount; i++) {
        std::memcpy(planes + offset, objects[i]->collider, objects[i]->cPlaneCount * sizeof(planeMtx));
        offset += objects[i]->cPlaneCount;
    }

    return { planes, totalPlanes };
}

void intializePEntityLocation(meshedObject& object) {

}
