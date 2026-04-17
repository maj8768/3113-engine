#include "../util.h"
#include <iostream>
#include <cmath>
#include <cstring>
#include "raylib.h"
#include "string"


int analyticalEdgeCollision(physicsEntity& player, planeMtx plane, vector3& applyAcc, planeMtx* collider, int collider_depth, float conservationPercent, float deltaTime, int& target, bool invertedNormals, bool& hasCollidedGround, bool& hasCollidedWall, int planeIndex, bool* edgeUsed) {

    vector3 qv[4];
    for (int k = 0; k < 4; k++)
        qv[k] = {plane.m[k][0], plane.m[k][1], plane.m[k][2]};

    vector3 rawN = cross3(qv[1] - qv[0], qv[3] - qv[0]);
    if (rawN.mag() < eps) return -1;
    vector3 quad_n = normalize3(rawN);
    vector3 quad_u = normalize3(qv[1] - qv[0]);
    vector3 quad_v = normalize3(qv[3] - qv[0]);

    const int MAX_V = 256;
    vector3 obbV[MAX_V];
    int ovc = 0;
    for (int i = 0; i < collider_depth && ovc + 4 <= MAX_V; i++)
        for (int j = 0; j < 4; j++)
            obbV[ovc++] = {collider[i].m[j][0], collider[i].m[j][1], collider[i].m[j][2]};

    vector3 obbAx[3];
    int nOA = 0;
    for (int i = 0; i < collider_depth && nOA < 3; i++) {
        vector3 fn = cross3(
            vector3{collider[i].m[1][0], collider[i].m[1][1], collider[i].m[1][2]} -
            vector3{collider[i].m[0][0], collider[i].m[0][1], collider[i].m[0][2]},
            vector3{collider[i].m[3][0], collider[i].m[3][1], collider[i].m[3][2]} -
            vector3{collider[i].m[0][0], collider[i].m[0][1], collider[i].m[0][2]}
        );
        if (fn.mag() < eps) continue;
        fn = normalize3(fn);
        bool dup = false;
        for (int k = 0; k < nOA; k++)
            if (fabsf(fabsf(dot3(fn, obbAx[k])) - 1.f) < 0.01f) { dup = true; break; }
        if (!dup) obbAx[nOA++] = fn;
    }
    if (nOA < 1) obbAx[nOA++] = {1.f, 0.f, 0.f};
    if (nOA < 2) obbAx[nOA++] = {0.f, 1.f, 0.f};
    if (nOA < 3) obbAx[nOA++] = {0.f, 0.f, 1.f};

    vector3 axes[15];
    int nAx = 0;
    axes[nAx++] = quad_n;
    axes[nAx++] = quad_u;
    axes[nAx++] = quad_v;
    for (int k = 0; k < 3; k++) axes[nAx++] = obbAx[k];
    vector3 qEdges[2] = { quad_u, quad_v };
    for (int qe = 0; qe < 2 && nAx < 15; qe++)
        for (int k = 0; k < 3 && nAx < 15; k++) {
            vector3 c = cross3(qEdges[qe], obbAx[k]);
            if (c.mag() > eps) axes[nAx++] = normalize3(c);
        }

    vector3 qCenter = (qv[0] + qv[1] + qv[2] + qv[3]).fmult(0.25f);
    vector3 oCenter = {0.f, 0.f, 0.f};
    for (int k = 0; k < ovc; k++) oCenter = oCenter + obbV[k];
    oCenter = oCenter.fmult(1.f / (float)ovc);

    float minOv = 1e9f;
    vector3 mtv  = quad_n;

    for (int a = 0; a < nAx; a++) {
        vector3 ax = axes[a];
        if (ax.mag() < eps) continue;
        ax = normalize3(ax);

        float qMin = 1e9f, qMax = -1e9f;
        for (int k = 0; k < 4; k++) {
            float p = dot3(qv[k], ax);
            qMin = fminf(qMin, p); qMax = fmaxf(qMax, p);
        }
        float oMin = 1e9f, oMax = -1e9f;
        for (int k = 0; k < ovc; k++) {
            float p = dot3(obbV[k], ax);
            oMin = fminf(oMin, p); oMax = fmaxf(oMax, p);
        }

        float overlap = fminf(qMax - oMin, oMax - qMin);
        if (overlap <= 0.f) return -1;

        if (overlap < minOv) {
            minOv = overlap;
            float sign = (dot3(oCenter - qCenter, ax) >= 0.f) ? 1.f : -1.f;
            mtv = ax.fmult(sign);
        }
    }

    bool isGround = fabsf(dot3(mtv, {0.f, 1.f, 0.f})) > 0.7f;

    if (isGround) {
        if (!hasCollidedGround) {
            player.location = player.location + mtv.fmult(minOv + eps);
            float vn = dot3(player.magnitude, mtv); if (vn < 0.f) player.magnitude = player.magnitude - mtv.fmult(vn);
            hasCollidedGround = true;
            player.groundPlane = planeIndex;
        }
        player.collidingY = true;
    } else {
        player.location = player.location + mtv.fmult(minOv + eps);
        float vn = dot3(player.magnitude, mtv);
        if (vn < 0.f) player.magnitude = player.magnitude - mtv.fmult(vn);
        hasCollidedWall = true;
    }

    return planeIndex;
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

    // for wall planes, also check 1 unit above the player's feet
    bool wallHitHigh = false;
    if (!isGround) {
        vector3 highPos = { player.location.x, player.location.y + 1.f, player.location.z };
        float tHigh = dot3(normal, highPos - p1) / n2;
        vector3 projHigh = highPos - normal.fmult(tHigh);
        vector3 tHigh2 = projHigh - p1;
        float dotwuH = dot3(tHigh2, u), dotwvH = dot3(tHigh2, v);
        float nYH = (dotwuH * dotv - dotwvH * dotuv) / d_check;
        float nZH = (dotwvH * dotu - dotwuH * dotuv) / d_check;
        if (nYH >= 0.f && nYH <= 1.f && nZH >= 0.f && nZH <= 1.f) {
            float sdHigh = dot3(highPos - close_point, normal) / normal.mag();
            if (sdHigh > -0.20f && sdHigh < 0.2f) wallHitHigh = true;
        }
    }

    if ((signedDist > -0.20f && signedDist < 0.2f) || wallHitHigh) {
//        std::cout << "COLLIDING" << std::endl;
        if (isGround) {
            if (!hasCollidedGround) {
                vector3 repos = close_point + normal.fmult(-0.2001f / normal.mag());
                if (signedDist > 0.f && repos.y > player.location.y) player.location.y = repos.y;
                hasCollidedGround = true;
                // std::cout << "standing on plane: " << planeIndex << std::endl;
                player.groundPlane = planeIndex;
            }
            player.collidingY = true;
        } else {
            if (!hasCollidedWall) {
                vector3 repos = close_point + normal.fmult(-0.2001f / normal.mag());
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

void applyRot(vector3 newRot, physicsEntity& pEntity) {
    pEntity.rot = pEntity.rot + newRot;
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

    if (collide) {
        pEntity.collidingY = false;
        constexpr float CULL_DIST = 250.0f;

        constexpr int MAX_COLLIDER_EDGES = 256;
        bool edgeUsed[MAX_COLLIDER_EDGES] = {};

        for (int wtc = 0; wtc < world.planeCount; wtc++) {
            const planeMtx& pl = world.planes[wtc];
            float cx = (pl.m[0][0] + pl.m[1][0] + pl.m[2][0] + pl.m[3][0]) * 0.25f;
            float cy = (pl.m[0][1] + pl.m[1][1] + pl.m[2][1] + pl.m[3][1]) * 0.25f;
            float cz = (pl.m[0][2] + pl.m[1][2] + pl.m[2][2] + pl.m[3][2]) * 0.25f;
            float dx = cx - pEntity.location.x;
            float dy = cy - pEntity.location.y;
            float dz = cz - pEntity.location.z;
            if (dx*dx + dy*dy + dz*dz > CULL_DIST * CULL_DIST) continue;

            if (pEntity.complexGeometry) {
                analyticalEdgeCollision(pEntity, world.planes[wtc], pEntity.applyAccel, collider, collider_depth, 1, deltaTime, target, invertedNormals, hasCollidedGround, hasCollidedWall, wtc, edgeUsed);
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
    if (pEntity.collidingY && pEntity.magnitude.y < 0.001f) pEntity.magnitude.y = 0.f;
//    std::cout << "player mag-y: " << pEntity.magnitude.y << std::endl;



    // lol no more terminal velocity...

//    // force terminal velocity
//    if (pEntity.magnitude.y > 214.f) {
//        pEntity.magnitude.y = 214.f; // roughly terminal velocity if a 3m sphere weighing 12500kg ;p
//    }
    (fabs(pEntity.magnitude.x) > 0.001f) ? pEntity.location.x += pEntity.magnitude.x * deltaTime : pEntity.magnitude.x = 0;
    (fabs(pEntity.magnitude.y) > 0.001f) ? pEntity.location.y += pEntity.magnitude.y * deltaTime : pEntity.magnitude.y = 0;
    (fabs(pEntity.magnitude.z) > 0.001f) ? pEntity.location.z += pEntity.magnitude.z * deltaTime : pEntity.magnitude.z = 0;

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
    pEntity.rot = {0.f, 0.f, 0.f};
    if (complexity == COMPLEX) {
        pEntity.complexGeometry = true;
    } else {
        pEntity.complexGeometry = false;
    }
}

void rotateX(vector3& v, float angle) {
    float cosA = cosf(angle);
    float sinA = sinf(angle);
    float y = v.y * cosA - v.z * sinA;
    float z = v.y * sinA + v.z * cosA;
    v.y = y;
    v.z = z;
}

void rotateY(vector3& v, float angle) {
    float cosA = cosf(angle);
    float sinA = sinf(angle);
    float x = v.x * cosA + v.z * sinA;
    float z = -v.x * sinA + v.z * cosA;
    v.x = x;
    v.z = z;
}

void rotateZ(vector3& v, float angle) {
    float cosA = cosf(angle);
    float sinA = sinf(angle);
    float x = v.x * cosA - v.y * sinA;
    float y = v.x * sinA + v.y * cosA;
    v.x = x;
    v.y = y;
}

void updateEntityRot(meshedObject& object) {

    for (int i = 0; i < object.mesh.count; i++) {
        for (int j = 0; j < 3; j++) {
            vector3 rot = object.pEntity.rot;
            vector3 vert = object.mesh.tris[i].v[j] - object.pEntity.location;
            rotateX(vert, rot.x);
            rotateY(vert, rot.y);
            rotateZ(vert, rot.z);
            object.mesh.tris[i].v[j] = vert + object.pEntity.location;
        }
    }
}

void updatePlayerColliderRot(player& player) {
    // camTarget.x is the yaw angle accumulated directly from mouse X input
    float yaw = player.camera.camTarget.x;

    for (int i = 0; i < player.cPlaneCount; i++) {
        for (int j = 0; j < 4; j++) {
            vector3 vert = {player.collider[i].m[j][0], player.collider[i].m[j][1], player.collider[i].m[j][2]};
            vert = vert - player.pEntity.location;
            rotateY(vert, -yaw);
            player.collider[i].m[j][0] = vert.x + player.pEntity.location.x;
            player.collider[i].m[j][1] = vert.y + player.pEntity.location.y;
            player.collider[i].m[j][2] = vert.z + player.pEntity.location.z;
        }
    }
}

void updatePlayerColliderLocation(player& player) {
    for (int i = 0; i < player.cPlaneCount; i++) {
        for (int v = 0; v < 4; v++) {
            player.collider[i].m[v][0] = player.colliderO[i].m[v][0] + player.pEntity.location.x;
            player.collider[i].m[v][1] = player.colliderO[i].m[v][1] + player.pEntity.location.y;
            player.collider[i].m[v][2] = player.colliderO[i].m[v][2] + player.pEntity.location.z;
        }
    }
    updatePlayerColliderRot(player);
}

void updateColliderRot(meshedObject& object) {

    for (int i = 0; i < object.cPlaneCount; i++) {
        for (int j = 0; j < 4; j++) {
            vector3 rot = object.pEntity.rot;
            vector3 colliderVert = {object.collider[i].m[j][0], object.collider[i].m[j][1], object.collider[i].m[j][2]};
            vector3 vert = colliderVert - object.pEntity.location;
            rotateX(vert, rot.x);
            rotateY(vert, rot.y);
            rotateZ(vert, rot.z);
            object.collider[i].m[j][0] = vert.x + object.pEntity.location.x;
            object.collider[i].m[j][1] = vert.y + object.pEntity.location.y;
            object.collider[i].m[j][2] = vert.z + object.pEntity.location.z;
        }
    }
}

void updateEntityLocation(meshedObject& object) {
    
    for (int i = 0; i < object.mesh.count; i++) {
        for (int j = 0; j < 3; j++) {
            float ox = object.mesh.trisO[i].v[j].x;
            float oy = object.mesh.trisO[i].v[j].y;
            float oz = object.mesh.trisO[i].v[j].z;

            object.mesh.tris[i].v[j].x = ox + object.pEntity.location.x;
            object.mesh.tris[i].v[j].y = oy + object.pEntity.location.y;
            object.mesh.tris[i].v[j].z = oz + object.pEntity.location.z;

            float onx = object.mesh.trisO[i].n[j].x;
            float ony = object.mesh.trisO[i].n[j].y;
            float onz = object.mesh.trisO[i].n[j].z;
            object.mesh.tris[i].n[j].x = onx;
            object.mesh.tris[i].n[j].y = ony;
            object.mesh.tris[i].n[j].z = onz;
        }
    }

    updateEntityRot(object);
}

void updateColliderLocation(meshedObject& object, player& player, bool playerObj) {
    if (playerObj) {
        updatePlayerColliderLocation(player);
    }
    else {
        for (int i = 0; i < object.cPlaneCount; i++) {
            for (int v = 0; v < 4; v++) {
                object.collider[i].m[v][0] = object.colliderO[i].m[v][0] + object.pEntity.location.x;
                object.collider[i].m[v][1] = object.colliderO[i].m[v][1] + object.pEntity.location.y;
                object.collider[i].m[v][2] = object.colliderO[i].m[v][2] + object.pEntity.location.z;
            }
        }
        updateColliderRot(object);
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


world buildWorld(meshedObject** objects, int objectCount, player& player) {
    static planeMtx* planes = nullptr;
    static int allocatedCount = 0;

    int totalPlanes = 0;
    for (int i = 0; i < objectCount; i++) {
        updateColliderLocation(*objects[i], player, false);
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
