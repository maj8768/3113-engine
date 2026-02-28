# 3113-engine

Project 2 Commit - [https://github.com/maj8768/3113-engine/commit/62669a301d9a41be9ae02cb52e662a442899d519](https://github.com/maj8768/3113-engine/commit/825eee9f7c7730b67de60041a50476ad408052c3)

Functionality:

* Full 2 player control
* Full 3D simulated physics
*   - Accels
*   - forces
*   - complex collision
* T-key single player when 1 ball
* Enter-key multiplayer for 1-3 ball
* End-screen on player-loss


![pongunfinished](https://github.com/maj8768/3113-engine/blob/main/ezgif-51b8f1666ff999e9.gif)




Project 1 Commit - https://github.com/maj8768/3113-engine/commit/13677a57c8dd263095c0faa105e63ea2b5d32bc8

Functionality:

![functionality](https://github.com/maj8768/3113-engine/blob/main/ezgif-51388f346d925919.gif)

3 objects are:
* 1. face 1 of pyramid
* 2. face 2 of pyramid
* 3. face 3 of pyramid
* 4. HM: face 4 of pyramid

Transforms:
* The camera orbits the pyramid/plane.
* The camera is still in screen space but constantly rotating in object space
* The faces are technically constantly scaling in screen space, but remain stationary in object space
* The entire pyramid translates along the y-axis in object space and because the camera has no-
* vertical movement it also translates along the y-axis in screen space!

Extra Credit:
* The background shifts from gray to white as the camera completes a full orbit!

Run with:

``sh build.sh``
