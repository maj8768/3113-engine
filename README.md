# 3113-engine

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
