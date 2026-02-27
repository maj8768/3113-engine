#include "util.h"
#include <iostream>
#include <cmath>

/**
 * Math from ChatGPT: https://chatgpt.com/share/69a0d1cd-2908-8001-a52c-c762d5f91148
 *
 *
 */
bool spherePlaneCollide(sphere_& sphere, planeMtx* plane, vector3& applyAcc, float conservationPercent, float deltaTime, bool q1) {

    const float eps = 1e-6;

    vector3 p1 = {plane->m[0][0], plane->m[0][1], plane->m[0][2]};
    vector3 p2 = {plane->m[1][0], plane->m[1][1], plane->m[1][2]};
    vector3 p3 = {plane->m[2][0], plane->m[2][1], plane->m[2][2]};
    vector3 p4 = {plane->m[3][0], plane->m[3][1], plane->m[3][2]};


    vector3 v1 = p2 - p1;
    vector3 v2 = p3 - p1;

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
        // std::cout << normal.fdiv(normalMag).y << std::endl;
        // std::cout << repos.x << ", " << repos.y << ", " << repos.z << std::endl;

        // std::cout << close_point.x << ", " << close_point.y << ", " << close_point.z << ", " << std::endl;
        sphere.location = repos;
        // std::cout << targetX << ", " << targetY << ", " << targetZ << std::endl;
        // std::cout << sphere.location.x << ", " << sphere.location.y << ", " << sphere.location.z << ", " << std::endl;
        // std::cout << ((sphere.location.x) < targetX) << std::endl;
        // if (fabs(sphere.location.x) < targetX) sphere.location.x = targetX;
        // if (fabs(sphere.location.y) < targetY) sphere.location.y = targetY;
        // if (fabs(sphere.location.z) < targetZ) sphere.location.z = targetZ;

        sphere.magnitude.x -= post_impact_vel.x * (2) * conservationPercent;
        sphere.magnitude.y -= post_impact_vel.y * (2) * conservationPercent;
        sphere.magnitude.z -= post_impact_vel.z * (2) * conservationPercent;
        // std::cout << post_impact_vel.x << ", " << post_impact_vel.y << ", " <<post_impact_vel.z << std::endl;
        // std::cout << distance << std::endl;

        // handle non-physics related collision
        (plane->action)(plane->id);
        // std::cout << "plane id: " << plane->id << std::endl;
        if (plane->id == 0) {
            // std::cout << "here 1" << std::endl;
            return false;
        }
        else {
            // sadness = plane->id;
            // std::cout << "1sadness: " << sadness << std::endl;
            // std::cout << "here 2" << std::endl;
            return true;
        }
        // std::cout << "xyz "  << plane->m[0][0] << ", " << plane->m[0][1] << ", "  << plane->m[0][2] << std::endl;
        // std::cout << "x "  << distance << std::endl;
        // return true;
    }
    return false;

    // float distance = sqrt(pow((x-sphere.location.x),2) + pow((y-sphere.location.y),2) + pow((z-sphere.location.z),2) );
    // // if (distance < sphere.size) {
    // //     for (int i = 0; i < sphere.accelForcesCount; i++) {
    // //         std::cout << "is not still (1=true): " << !(std::abs(sphere.magnitude.y) < .05) << " at: " << sphere.magnitude.y << std::endl;
    // //         if (!(std::abs(sphere.magnitude.y) < .05)) {
    // //             sphere.location.y += (-std::abs(sphere.accelForces[i].y)/sphere.accelForces[i].y) *.01 + plane.m[0][1];
    // //             sphere.magnitude.y = -sphere.magnitude.y;
    // //         }
    // //         else {
    // //             std::cout << "still" << std::endl;
    // //             sphere.location.y = plane.m[0][1]; // at rest
    // //         }
    // //     }
    // // }
    // if (distance < sphere.size) {
    //     std::cout << "is not still (1=true): " << !(std::abs(sphere.magnitude.y) < .25 - sphere.accelForces[0].y * deltaTime) << " at: " << sphere.magnitude.y - sphere.accelForces[0].y * deltaTime<< std::endl;
    //     const float planeX = plane.m[0][0];
    //     const float planeY = plane.m[0][1];
    //     const float planeZ = plane.m[0][2];
    //     const float restSpeed = 0.25f;
    //     const float eps = 0.001f;
    //
    //     std::cout << x << ", " << y << ", " << z << std::endl;
    //     std::cout << cos(atan2(yDisp,zDisp)) << std::endl;
    //
    //     float targetX = x + sphere.size + eps;
    //     float targetY = y + sphere.size + eps;
    //     float targetZ = z + sphere.size + eps;
    //
    //     // snap out of penetration first
    //     if (sphere.location.x < targetX) sphere.location.x = targetX;
    //     if (sphere.location.y < targetY) sphere.location.y = targetY;
    //     if (sphere.location.z < targetZ) sphere.location.z = targetZ;
    //
    //     // only bounce if moving down into the plane
    //     if (std::abs(sphere.magnitude.x) > restSpeed - sphere.accelForces[0].x * deltaTime /* counters applied acceleration in past frame (allows it to reach rest)*/) {
    //         // sphere.magnitude.x = -sphere.magnitude.x * conservationPercent; // lol i dont do math
    //     } else {
    //         // sphere.magnitude.x = 0.0f;
    //         // applyAcc.x = 0;
    //     }
    //     if (std::abs(sphere.magnitude.y) > restSpeed - sphere.accelForces[0].y * deltaTime /* counters applied acceleration in past frame (allows it to reach rest)*/) {
    //         sphere.magnitude.y = -sphere.magnitude.y * conservationPercent; // lol i dont do math
    //     } else {
    //         sphere.magnitude.y = 0.0f;
    //         applyAcc.y = 0;
    //     }
    //     if (std::abs(sphere.magnitude.z) > restSpeed - sphere.accelForces[0].z * deltaTime /* counters applied acceleration in past frame (allows it to reach rest)*/) {
    //         // sphere.magnitude.z = -sphere.magnitude.z * conservationPercent; // lol i dont do math
    //     } else {
    //         // sphere.magnitude.z = 0.0f;
    //         // applyAcc.z = 0;
    //     }
    // }
    // return distance < sphere.size;
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

bool processPhysics(float deltaTime, int frameRate, sphere_& sphere, world& world, bool collisionOnly) {
    // std::cout << "appfs" << std::endl;
    bool collide = false;
    bool acc = false;

    // collision

    // std::cout << "applying forces: " << sphere.magnitude.y << std::endl;
    if (collisionOnly){
        for (int itc = 0; itc < world.planeCount; itc++) {
            if (collide == 0) {
                collide = (spherePlaneCollide(sphere, world.planes[itc], sphere.applyAccel, 1, deltaTime, true));
                // std::cout << "collide-only: " << collide << std::endl;
                // std::cout << "r: " << world.planes[itc]->m[0][1] << std::endl;
            }
        }
        // std::cout << "collision only: " << collide << std::endl;
        return collide;
    }
    else {
        for (int itc = 0; itc < world.planeCount; itc++) {
            if (collide == 0) {
                collide = (spherePlaneCollide(sphere, world.planes[itc], sphere.applyAccel, 1, deltaTime, false));
                // std::cout<< (*world.planes[itc]).m[0][1] << std::endl;
                // std::cout << "full-world: " << collide << std::endl;
            }
        }
        // force transfer
        // std::cout << sphere.magnitude.y << ", " << sphere.magnitude.x << std::endl;
        // std::cout << sphere.magnitude.z << std::endl;
        if (sphere.newForce.x !=0 || sphere.newForce.y !=0 || sphere.newForce.z !=0 || acc == true) {
            sphere.magnitude.x += sphere.newForce.x;
            sphere.magnitude.y += sphere.newForce.y;
            sphere.magnitude.z += sphere.newForce.z;
            
            sphere.newForce.murder();
        }

        // acceleration

        if (sphere.accelForcesCount != 0) {
            acc = true;
            for (int i = 0; i < sphere.accelForcesCount; i++) {
                // std::cout << "delta: " << deltaTime << std::endl;
                
                sphere.magnitude.x += sphere.accelForces[i].x * sphere.applyAccel.x * deltaTime;
                sphere.magnitude.y += sphere.accelForces[i].y * sphere.applyAccel.y * deltaTime;
                sphere.magnitude.z += sphere.accelForces[i].z * sphere.applyAccel.z * deltaTime;
            }
        }

        // std::cout << sphere.magnitude.y << std::endl;

        // if (std::abs(sphere.magnitude.x) < .25) {
        //     sphere.magnitude.x = 0;
        // }
        // if (std::abs(sphere.magnitude.y) < .25) {
        //     sphere.magnitude.y = 0;
        // }
        // if (std::abs(sphere.magnitude.z) < .25) {
        //     sphere.magnitude.z = 0;
        // }


        // std::cout<<  sphere.magnitude.y * deltaTime << ", " << sphere.applyAccel.y << std::endl;

        sphere.location.x += sphere.magnitude.x * deltaTime;
        sphere.location.y += sphere.magnitude.y * deltaTime;
        sphere.location.z += sphere.magnitude.z * deltaTime;

        return collide;
    }
}
