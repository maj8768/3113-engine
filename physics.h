void applyForce(vector3 newForce, sphere_& sphere);

void applyAcceleration(vector3 newAccel, sphere_& sphere);

bool processPhysics(float deltaTime, int frameRate, sphere_& sphere, world& plane, bool collisionOnly);
