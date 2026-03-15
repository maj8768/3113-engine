#include "../util.h"
#include <iostream>
#include <cmath>

/**
 * Math from ChatGPT: https://chatgpt.com/share/69a0d1cd-2908-8001-a52c-c762d5f91148
 *
 *
 */
bool spherePlaneCollide(physicsEntity& player, planeMtx plane, vector3& applyAcc, float conservationPercent, float deltaTime, int& target, bool invertedNormals) {
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

    if (normedYD < 0.f) {
        normedYD = 0.f;
    }
    else if (normedYD > 1.f) {
        normedYD = 1.f;
    }
    if (normedZD < 0.f) {
        normedZD = 0.f; 
    }
    else if (normedZD > 1.f) {
        normedZD = 1.f;
    }

    vector3 close_point = p1 + u.fmult(normedYD) + v.fmult(normedZD); // p1 coz its with respect to the plane
    float signedDist = dot3(player.location - close_point, normal) / normal.mag();
    
    // old but still works

    // std::cout << "here" << std::endl;
    // std::cout << distance << std::endl;
    if (signedDist > -0.2f && signedDist < 0.2f) {

        vector3 repos = close_point + normal.fmult(-0.20001f / normal.mag());

        player.location.x = repos.x;
        player.location.z = repos.z;

        // player.magnitude.x -= post_impact_vel.x * (2) * conservationPercent;
        // player.magnitude.y -= post_impact_vel.y * (2) * conservationPercent;
        // player.magnitude.z -= post_impact_vel.z * (2) * conservationPercent; 
        player.magnitude.x -= post_impact_vel.x * (1) * conservationPercent;
        player.magnitude.y -= post_impact_vel.y * (1) * conservationPercent;
        player.magnitude.z -= post_impact_vel.z * (1) * conservationPercent; 

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
    if (pEntity.accelForcesCount < pEntity.maxAccelForces) {
        pEntity.accelForces[pEntity.accelForcesCount] = newAccel;
        pEntity.accelForcesCount += 1;
        std::cout << pEntity.accelForcesCount << std::endl;
        std::cout << "you have applied: " << pEntity.accelForcesCount << " forces." <<std::endl;
    }
    else {
        std::cout << "maximum number of constant accels applied, you are probably misusing this." << std::endl;
        std::cout << "you have applied: " << pEntity.maxAccelForces << " forces." <<std::endl;
    }
}

void processPhysics(float deltaTime, int frameRate, physicsEntity& pEntity, world& world, bool& end, int& target, bool invertedNormals, bool collide) {
    bool acc = false;
    
    // checking flat collision for each plane in the world (probably should dynamically build this)
    if (collide) {
        for (int wtc = 0; wtc < world.planeCount; wtc++) {
            // std::cout << wtc << std::endl;
            (spherePlaneCollide(pEntity, world.planes[wtc], pEntity.applyAccel, 1, deltaTime,target, invertedNormals));
        }
    }
    // force transfer

    if (pEntity.newForce.x !=0 || pEntity.newForce.y !=0 || pEntity.newForce.z !=0 || acc == true) {
        pEntity.magnitude.x += pEntity.newForce.x * deltaTime;
        pEntity.magnitude.y += pEntity.newForce.y * deltaTime;
        pEntity.magnitude.z += pEntity.newForce.z * deltaTime;
        
        pEntity.newForce.murder();
    }
    // acceleration
    if (pEntity.accelForcesCount != 0) {
        acc = true;
        for (int i = 0; i < pEntity.accelForcesCount; i++) {
            // std::cout << "delta: " << deltaTime << std::endl;
//            std::cout << pEntity.applyAccel.y << std::endl;
            pEntity.magnitude.x += pEntity.accelForces[i].x * pEntity.applyAccel.x * deltaTime;
            pEntity.magnitude.y += pEntity.accelForces[i].y * pEntity.applyAccel.y * deltaTime;
            pEntity.magnitude.z += pEntity.accelForces[i].z * pEntity.applyAccel.z * deltaTime;
        }
    }
    if (pEntity.magnitude.y > 214.f) {
        pEntity.magnitude.y = 214.f; // roughly terminal velocity if a 3m sphere ;p
    }
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
void initializePhysicsEntity(physicsEntity& pEntity, int maxAccelForces) {
    pEntity.accelForces = new vector3[maxAccelForces];
    pEntity.maxAccelForces = maxAccelForces;
    pEntity.accelForcesCount = 0;
    pEntity.applyAccel = {1.f, 1.f, 1.f};
    pEntity.newForce = {0.f, 0.f, 0.f};
    pEntity.magnitude = {0.f, 0.f, 0.f};
}

void updateEntityLocation(meshedObject& object) {
//    std::cout << object.pEntity.location.y << std::endl;
//    std::cout << object.mesh.trisO[0].v[1] << std::endl;
    for (int i = 0; i < object.mesh.count; i++) {
        for (int j = 0; j < 3; j++) {
            object.mesh.tris[i].v[j].x = object.mesh.trisO[i].v[j].x + object.pEntity.location.x;
            object.mesh.tris[i].v[j].y = object.mesh.trisO[i].v[j].y + object.pEntity.location.y;
            object.mesh.tris[i].v[j].z = object.mesh.trisO[i].v[j].z + object.pEntity.location.z;
        }
    }
//    std::cout << object.mesh.tris[0].v[1] << std::endl;
    
}

void intializePEntityLocation(meshedObject& object) {
    
}
