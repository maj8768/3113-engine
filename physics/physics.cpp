#include "../util.h"
#include <iostream>
#include <cmath>

/**
 * Math from ChatGPT: https://chatgpt.com/share/69a0d1cd-2908-8001-a52c-c762d5f91148
 *
 *
 */
bool spherePlaneCollide(sphere_& sphere, planeMtx* plane, vector3& applyAcc, float conservationPercent, float deltaTime, int& target) {

    vector3 p1 = {plane->m[0][0], plane->m[0][1], plane->m[0][2]};
    vector3 p2 = {plane->m[1][0], plane->m[1][1], plane->m[1][2]};
    vector3 p3 = {plane->m[2][0], plane->m[2][1], plane->m[2][2]};
    vector3 p4 = {plane->m[3][0], plane->m[3][1], plane->m[3][2]};


    vector3 v1 = p2 - p1;
    vector3 v2 = p4 - p1;

    vector3 normal = cross3(v1, v2);

    // new math for closest point on finite quad and NOT infinite plane ! very important

    float n2 = dot3(normal, normal);
    if (n2 < eps) return false;

    vector3 post_impact_vel = normal.fmult(dot3(sphere.magnitude, normal) / n2);

    float tplane = dot3(normal, sphere.location - p1) / n2;
    vector3 proj = sphere.location - normal.fmult(tplane);

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
    float distance = close_point.dist(sphere.location);
    
    // old but still works

    if (distance < sphere.size) {

        vector3 repos = close_point + normal.fmult(sphere.size / normal.mag());

        sphere.location = repos;

        sphere.magnitude.x -= post_impact_vel.x * (2) * conservationPercent;
        sphere.magnitude.y -= post_impact_vel.y * (2) * conservationPercent;
        sphere.magnitude.z -= post_impact_vel.z * (2) * conservationPercent; 

        // handle non-physics related collision
        (plane->action)(plane->id);
        target = plane->id;

        if (plane->id == 0) {

            return false;
        }
        else {
            return true;
        }
    }
    return false;
}

void applyForce(vector3 newForce, sphere_& sphere) {
    sphere.newForce = newForce;
}

void applyAcceleration(vector3 newAccel, sphere_& sphere) {
    if (sphere.accelForcesCount < sphere.maxAccelForces) {
        sphere.accelForces[sphere.accelForcesCount] = newAccel;
        sphere.accelForcesCount += 1;
        std::cout << "you have applied: " << sphere.accelForcesCount << " forces." <<std::endl;
    }
    else {
        std::cout << "maximum number of constant accels applied, you are probably misusing this." << std::endl;
        std::cout << "you have applied: " << sphere.maxAccelForces << " forces." <<std::endl;
    }
}

void processPhysics(float deltaTime, int frameRate, player& player, world& world, bool& end, int& target) {

    bool acc = false;

    // checking flat collision for each plane in the world (probably should dynamically build this)
//    for (int wtc = 0; wtc < world.planeCount; wtc++) {
//        (spherePlaneCollide(sphere, world.planes[wtc], sphere.applyAccel, 1, deltaTime,target));
//    }
    // force transfer

    if (player.newForce.x !=0 || player.newForce.y !=0 || player.newForce.z !=0 || acc == true) {
        player.magnitude.x += player.newForce.x;
        player.magnitude.y += player.newForce.y;
        player.magnitude.z += player.newForce.z;
        
        player.newForce.murder();
    }
    // acceleration
    if (player.accelForcesCount != 0) {
        acc = true;
        for (int i = 0; i < player.accelForcesCount; i++) {
            // std::cout << "delta: " << deltaTime << std::endl;
            
            player.magnitude.x += player.accelForces[i].x * player.applyAccel.x * deltaTime;
            player.magnitude.y += player.accelForces[i].y * player.applyAccel.y * deltaTime;
            player.magnitude.z += player.accelForces[i].z * player.applyAccel.z * deltaTime;
        }
    }

    player.location.x += player.magnitude.x * deltaTime;
    player.location.y += player.magnitude.y * deltaTime;
    player.location.z += player.magnitude.z * deltaTime;

}
