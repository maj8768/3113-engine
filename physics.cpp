#include "util.h"
#include <iostream>
#include <cmath>

/**
 * MDN - Game Techniques (3D collision)
 */
bool spherePlaneCollide(sphere_& sphere, planeMtx& plane, vector3& applyAcc) {
    float x = std::max(plane.m[0][0],std::min(sphere.location.x, plane.m[2][0]));
    float y = std::max(plane.m[0][1],std::min(sphere.location.y, plane.m[2][1]));
    float z = std::max(plane.m[0][2],std::min(sphere.location.z, plane.m[2][2]));

    float distance = sqrt(pow((x-sphere.location.x),2) + pow((y-sphere.location.y),2) + pow((z-sphere.location.z),2) );
    // if (distance < sphere.size) {
    //     for (int i = 0; i < sphere.accelForcesCount; i++) {
    //         std::cout << "is not still (1=true): " << !(std::abs(sphere.magnitude.y) < .05) << " at: " << sphere.magnitude.y << std::endl;
    //         if (!(std::abs(sphere.magnitude.y) < .05)) {
    //             sphere.location.y += (-std::abs(sphere.accelForces[i].y)/sphere.accelForces[i].y) *.01 + plane.m[0][1];
    //             sphere.magnitude.y = -sphere.magnitude.y;
    //         }
    //         else {
    //             std::cout << "still" << std::endl;
    //             sphere.location.y = plane.m[0][1]; // at rest
    //         }
    //     }
    // }
    if (distance < sphere.size) {
        std::cout << "is not still (1=true): " << !(std::abs(sphere.magnitude.y) < .25) << " at: " << sphere.magnitude.y << std::endl;
        const float planeY = plane.m[0][1];
        const float restSpeed = 0.25f;
        const float eps = 0.001f;

        float targetY = planeY + sphere.size + eps;

        // snap out of penetration first
        if (sphere.location.y < targetY) sphere.location.y = targetY;

        // only bounce if moving down into the plane
        if (std::abs(sphere.magnitude.y) > restSpeed) {
            sphere.magnitude.y = -sphere.magnitude.y;
        } else {
            sphere.magnitude.y = 0.0f;
            applyAcc.y = 0;
        }
    }
    return distance < sphere.size;
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

void processPhysics(float deltaTime, int frameRate, sphere_& sphere, planeMtx& plane) {
    // std::cout << "appfs" << std::endl;
    bool acc = false;

    // collision

    spherePlaneCollide(sphere, plane, sphere.applyAccel);

    // acceleration

    if (sphere.accelForcesCount != 0) {
        acc = true;
        for (int i = 0; i < sphere.accelForcesCount; i++) {
            // std::cout << "accel: " << i << std::endl;
            sphere.magnitude.x += sphere.accelForces[i].x * deltaTime * sphere.applyAccel.x;
            sphere.magnitude.y += sphere.accelForces[i].y * deltaTime * sphere.applyAccel.y;
            sphere.magnitude.z += sphere.accelForces[i].z * deltaTime * sphere.applyAccel.z;
        }
    }

    // force transfer
    //std::cout << sphere.magnitude.y << std::endl;
    if (sphere.newForce.x !=0 || sphere.newForce.y !=0 || sphere.newForce.z !=0 || acc == true) {
        sphere.magnitude.x += sphere.newForce.x;
        sphere.magnitude.y += sphere.newForce.y;
        sphere.magnitude.z += sphere.newForce.z;

        sphere.newForce.murder();
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


    sphere.location.x += sphere.magnitude.x * deltaTime;
    sphere.location.y += sphere.magnitude.y * deltaTime;
    sphere.location.z += sphere.magnitude.z * deltaTime;
}
