void movePlayer(player& player1, bool swappedNormals);
void moveLook(player& player1, float xDelta, float yDelta);
void playerFollowBall(player& player1, sphere_ ball, bool swappedNormals);

void standardCollide(int id);
void killPlayer(int id);
void paddleHit(int id);
