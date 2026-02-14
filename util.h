#include "raylib.h"

struct triangleMtx {
    float x1, y1, z1;
    float x2, y2, z2;
    float x3, y3, z3;
    Color color;
};

struct pyramidMtx {
    float m[4][3];
    Color color;
    float textureArea[4][6];
    Texture2D texture;
};

struct planeMtx {
    float m[4][3];
    Color color;
    float textureArea[4][2];
    Texture2D texture;
};

struct vector4 {
    float x, y, z, t;
};

struct vector3 {
    float x, y, z;
};

struct vector2 {
    float x, y;
};

struct mtx44 {
    float m[4][4];
};

struct camera {
    vector3 camPos;
    vector3 camTarget;
    vector3 up;
    float aspect;
    float fov;
};

mtx44 mmult4(const mtx44&, const mtx44&);

vector4 modmmult(const mtx44&, const vector4&);

float dot3(const vector3&, const vector3&);

vector3 cross3(const vector3&, const vector3&);

float len3(const vector3&);

vector3 normalize3(const vector3&);

vector4 extendV3(const vector3&);

float lerp_sin(float, float, float);