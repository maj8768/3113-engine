#include "raylib.h"
#include <cmath>

// Global Constants
static constexpr int SCREEN_WIDTH        = 400 * 1.5f,
              SCREEN_HEIGHT       = 300 * 1.5f,
              FPS                 = 60;

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

/**
 * m[0][~] and m[2][~] are min/max
 */
struct planeMtx {
    float m[4][3];
    Color color;
    // float textureArea[4][2];
    Texture2D texture;
    int id;
    void (*action)(int);
};

struct vector4 {
    float x, y, z, t;
    void murder() { x = 0.0f; y = 0.0f; z = 0.0f; t = 0.0f; }
};

struct vector3 {
    float x, y, z;
    void murder() { x = 0.0f; y = 0.0f; z = 0.0f; };

    vector3(float x = 0, float y = 0, float z = 0) : x(x), y(y), z(z) {}

    vector3 operator+(const vector3& other) const {
        return {x + other.x, y + other.y, z + other.z};
    }

    vector3 operator-(const vector3& other) const {
        return {x - other.x, y - other.y, z - other.z};
    }
    vector3 operator*(const vector3& other) const {
        return {x * other.x, y * other.y, z * other.z};
    }
    vector3 operator/(const vector3& other) const {
        return {x / other.x, y / other.y, z / other.z};
    }
    vector3 operator^(const float other) const {
        return {(std::pow(x,other)), std::pow(y,other), std::pow(z,other)};
    }

    vector3& operator+=(const vector3& o) { x+=o.x; y+=o.y; z+=o.z; return *this; }

    [[nodiscard]] vector3 fmult(const float other) const {
        return {x * other, y * other, z * other};
    }

    [[nodiscard]] vector3 fadd(const float other) const {
        return {x + other, y + other, z + other};
    }

    [[nodiscard]] vector3 fsub(const float other) const {
        return {x - other, y - other, z - other};
    }

    [[nodiscard]] vector3 fdiv(const float other) const {
        return {x / other, y / other, z / other};
    }

    [[nodiscard]] float dist(const vector3& other) const {
        return static_cast<float>(sqrtf(
            std::pow(other.x - x, 2) + std::pow(other.y - y, 2) + std::pow(other.z - z, 2)
        ));
    }

    [[nodiscard]] float mag() const {
        return static_cast<float>(sqrtf(
            std::pow(x, 2) + std::pow(y, 2) + std::pow(z, 2)
        ));
    }
};

struct vector2 {
    float x, y;
    void murder() { x = 0.0f; y = 0.0f; }
};

struct mtx44 {
    float m[4][4];
};

struct gonalMtx {
    vector3* mtx;
    int size;
};

struct spungonMtx {
    gonalMtx* mtxarr;
    int size;
};

struct sphere_ {
    spungonMtx spungon_mtx;
    vector3 location;
    int depth;
    float size;
    vector3 magnitude;
    vector3 newForce;
    vector3* accelForces;
    int maxAccelForces;
    int accelForcesCount;
    vector3 applyAccel;
};

struct camera {
    vector3 camPos;
    vector3 camTarget;
    vector3 up;
    float aspect;
    float fov;
};

// temp player for pong (should change for 3d movement + cam)
struct player {
    vector3 location;
    planeMtx* model;
    planeMtx bounding;
    vector4 controls;
    int hits;
};

struct world {
    planeMtx** planes;
    int planeCount;
};

struct worldsStore {
    world worldInstance;
    world playerInstance;
};

mtx44 mmult4(const mtx44&, const mtx44&);

vector4 modmmult(const mtx44&, const vector4&);

float dot3(const vector3&, const vector3&);

vector3 cross3(const vector3&, const vector3&);

float len3(const vector3&);

vector3 normalize3(const vector3&);

vector3 extendV2(const vector2&);

vector4 extendV3(const vector3&);

void calculateGon2D(const int n, gonalMtx& out, const bool vertical, const int size);

void spinGon2D(spungonMtx& out, const float size);

float epsCheck(float val, float eps);
