void movePlayer(gameData& gData, player& player1, bool swappedNormals, float deltaTime, float maxSpeed);
void moveLook(player& player1, float deltaTime, vector2 md);
void haltPlayerLerp(player& player, bool swappedNormals, float deltaTime);
void playerFollowBall(player& player1, sphere_ ball, bool swappedNormals);

void standardCollide(int id);
void killPlayer(int id);
void paddleHit(int id);
