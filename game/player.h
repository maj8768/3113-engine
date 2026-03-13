void movePlayer(player& player1, bool swappedNormals, float deltaTime);
void moveLook(player& player1, float deltaTime, vector2 md);
void playerFollowBall(player& player1, sphere_ ball, bool swappedNormals);

void standardCollide(int id);
void killPlayer(int id);
void paddleHit(int id);
