bool spherePlaneCollide(physicsEntity& player, planeMtx plane, vector3& applyAcc, float conservationPercent, float deltaTime, int& target, bool invertedNormals, bool& hasCollidedGround, bool& hasCollidedWall, int planeIndex);

void applyForce(vector3 newForce, physicsEntity& pEntity);

void applyAcceleration(vector3 newAccel, physicsEntity& pEntity);

void processPhysics(float deltaTime, int frameRate, physicsEntity& pEntity, world& world, bool& end, int& target, bool invertedNormals, bool collide, planeMtx* collider, int collider_depth, bool* collidedOut = nullptr, vector3* collisionPointOut = nullptr);

void initializePhysicsEntity(physicsEntity& pEntity, float weight, physicsComplexity complexity);

void updateEntityLocation(meshedObject& object);

void updateColliderLocation(meshedObject& object, player& player, bool playerObj);

void resetMeshedLocation(meshedObject& object);

void buildWorld(world& w, meshedObject** objects, int objectCount);

// Raycast against the world's collider quads. Returns the distance to the nearest quad
// hit along `dir` (unit) from `origin`, clamped to maxDist (maxDist if nothing is hit).
float raycastWorld(vector3 origin, vector3 dir, float maxDist, world& w);

