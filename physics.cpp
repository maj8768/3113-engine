#include "util.h"
#include <iostream>
#include <cmath>

void processPhysics(float deltaTime, int frameRate, sphere_& sphere) {

    bool acc = false;

    // acceleration

    if (sphere.accelForcesCount != 0) {
        acc = true;
        for (int i = 0; i < sphere.accelForcesCount; i++) {
            sphere.magnitude.x += sphere.accelForces[i].x * deltaTime;
            sphere.magnitude.y += sphere.accelForces[i].y * deltaTime;
            sphere.magnitude.z += sphere.accelForces[i].z * deltaTime;
        }
    }

    // force transfer
    std::cout << sphere.magnitude.y << std::endl;
    if (sphere.newForce.x !=0 || sphere.newForce.y !=0 || sphere.newForce.z !=0 || acc == true) {
        sphere.magnitude.x += sphere.newForce.x;
        sphere.magnitude.y += sphere.newForce.y;
        sphere.magnitude.z += sphere.newForce.z;

        sphere.newForce.murder();

        sphere.location.x += sphere.magnitude.x * deltaTime;
        sphere.location.y += sphere.magnitude.y * deltaTime;
        sphere.location.z += sphere.magnitude.z * deltaTime;
    }
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