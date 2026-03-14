void applyForce(vector3 newForce, sphere_& sphere);

void applyAcceleration(vector3 newAccel, sphere_& sphere);

void processPhysics(float deltaTime, int frameRate, player& player, world& world, bool& end, int& target, bool invertedNormals);
