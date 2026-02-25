#include "util.h"
#include <iostream>
#include <cmath>

/**
 * Math from ChatGPT: https://chatgpt.com/share/6999099c-52d0-8001-98f7-31b3df5aa762
 *
 *
 */
bool spherePlaneCollide(sphere_& sphere, planeMtx& plane, vector3& applyAcc, float conservationPercent, float deltaTime) {

    const float eps = 1e-6;

    vector3 p1 = {plane.m[0][0], plane.m[0][1], plane.m[0][2]};
    vector3 p2 = {plane.m[1][0], plane.m[1][1], plane.m[1][2]};
    vector3 p3 = {plane.m[2][0], plane.m[2][1], plane.m[2][2]};

    vector3 v1 = p2 - p1;
    vector3 v2 = p3 - p1;

    vector3 normal = cross3(v1, v2);

    float angleX = epsCheck(normal.y != 0 ? cos(atan2(normal.y, normal.x)) : 0.f,eps);
    float angleZ = epsCheck(normal.y != 0 ? cos(atan2(normal.y, normal.z)) : 0.f,eps);
    float angleYX = normal.x != 0 ? normal.y / normal.x : 0.f;
    float angleYZ = normal.z != 0 ? normal.y / normal.z : 0.f;

    // std::cout << angleX << ", " << angleZ << ", " << angleYX << ", " << angleYZ << std::endl;

    vector3 post_impact_vel = normal.fmult((dot3(sphere.magnitude,normal)/std::pow(normal.mag(),2)));
    // std::cout << dot3(sphere.magnitude,normal) << std::endl;
    // std::cout << normalMag << std::endl;
    // std::cout << sphere.magnitude.x << ", " << sphere.magnitude.y << ", " << sphere.magnitude.z << ", " << std::endl;
    // std::cout << post_impact_vel.x << ", " << post_impact_vel.y << ", " << post_impact_vel.z << ", " << std::endl;

    float tops = normal.x*sphere.location.x + normal.y*sphere.location.y + normal.z*sphere.location.z - dot3(normal,p1);
    vector3 close_point = sphere.location - normal.fmult(tops/((normal^2).x + (normal^2).y + (normal^2).z));
    float distance = close_point.dist(sphere.location);
    // std::cout << close_point.x << ", " << close_point.y << ", " << close_point.z << ", " << std::endl;
    // std::cout << "distance: " << (distance) << std::endl;

    if (distance < sphere.size) {
        vector3 repos = close_point + normal.fmult(sphere.size / normal.mag());
        // std::cout << normal.fdiv(normalMag).y << std::endl;
        std::cout << repos.x << ", " << repos.y << ", " << repos.z << std::endl;

        std::cout << close_point.x << ", " << close_point.y << ", " << close_point.z << ", " << std::endl;
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
        std::cout << distance << std::endl;
        return true;
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

void processPhysics(float deltaTime, int frameRate, sphere_& sphere, world& world) {
    // std::cout << "appfs" << std::endl;
    bool acc = false;

    // collision

    // std::cout << "applying forces: " << sphere.magnitude.y << std::endl;
    for (int itc = 0; itc < world.planeCount; itc++) {
        (spherePlaneCollide(sphere, world.planes[itc], sphere.applyAccel, 1, deltaTime));
    }

    // force transfer
    std::cout << sphere.magnitude.y << ", " << sphere.magnitude.x << std::endl;
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
}
