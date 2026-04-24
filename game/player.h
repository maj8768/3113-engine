void movePlayer(gameData& gData, player& player1, meshedObject& car, bool swappedNormals, float deltaTime, float maxSpeed, Sound js);
void moveLook(player& player1, float deltaTime, vector2 md);
void haltPlayerLerp(player& player, bool swappedNormals, float deltaTime);
void playerFollowBall(player& player1, sphere_ ball, bool swappedNormals);

void getStartingInput(gameData& gData);
void getRestartInput(gameData& gData);
void getEscapeMenuInput(gameData& gData);

void standardCollide(int id);
void killPlayer(int id);
void paddleHit(int id);
