bool spherePlaneCollide(physicsEntity& player, planeMtx plane, vector3& applyAcc, float conservationPercent, float deltaTime, int& target, bool invertedNormals, bool& hasCollidedGround, bool& hasCollidedWall, int planeIndex);

void applyForce(vector3 newForce, physicsEntity& pEntity);

void applyAcceleration(vector3 newAccel, physicsEntity& pEntity);

void processPhysics(float deltaTime, int frameRate, physicsEntity& pEntity, world& world, bool& end, int& target, bool invertedNormals, bool collide);

void initializePhysicsEntity(physicsEntity& pEntity, float weight);

void updateEntityLocation(meshedObject& object);

void updateColliderLocation(meshedObject& object);

void resetMeshedLocation(meshedObject& object);

world buildWorld(meshedObject** objects, int objectCount);


