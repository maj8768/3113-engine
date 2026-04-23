#pragma once
#include "raylib.h"
#include <cmath>

#ifndef M_PI
    #define M_PI 3.14159
#endif

// Global Constants
constexpr int SCREEN_WIDTH        = 1280,
              SCREEN_HEIGHT       = 720,
                FPS               = 0;
                
constexpr float eps = 1e-6;

enum physicsComplexity {
    SIMPLE,
    COMPLEX
};

extern bool debugMode;

enum buyState {
    START,
    GOODBYE,
    DRANK_SELECT,
    CIG_SELECT,
    GOODBYE_SELECT,
    BACK_SELECT,
    DRANK,
    CIG,
    BUY_DRANK,
    BUY_CIG,
    THANKS,
    POOR,
};

enum buySelection {
        ONE,
        TWO,
        THREE
};

struct buyMenu {
    buyState state;
    buySelection selection;
    int optionCount = 2;
};

extern buyMenu bs;

struct playerState {
    bool canCar;
    bool inCar;
    float drunkenness;
    float cigaretteTimer;
    float carFuel;
    float leftTurn = 0.f;
    float rightTurn = 0.f;
    float brake = 0.f;
    float forward = 0.f;
    float money = 100.f;
    bool shiftUp = false;
    bool shiftDown = false;
    bool reverseDown  = false;
    bool canGasPump = false;
    bool hasGasPump = false;
    bool pumpingUp = false;
    bool canPickDrank = false;
    bool hasDrank = false;
    bool hasCig = false;
    bool canBuy = false;
    bool buying = false;
    bool canDrinkDrank = false;
    bool canSmokeCig = false;
    bool noClip = false;

};

struct gameData {
    
    float fadeTo;
    bool isDying;

    bool gameStarted;
    bool gameEnded;
    bool infommercial;

    bool bgMusicLevel1;
    bool bgMusicLevel2;
    bool bgMusicLevel3;

    bool hasAudioLevel1;
    bool hasAudioLevel2;
    bool hasAudioLevel3;

    bool hasChairScared;

    enum levels {
        LEVEL1,
        LEVEL2,
        LEVEL3,
        GAMEEND,
        GAMEWIN,
        GAMESTART,
        GAMEINFOMERCIAL,
        TESTING_ENVIRONMENT
    } currentLevel;

    int lives;

};

struct triangleMtx {
    float x1, y1, z1;
    float x2, y2, z2;
    float x3, y3, z3;
    Color color;
};

struct shaderStore {
    Shader shader;
    int shaderloc;
    int colorLoc;
    int lightDirLoc;
    int lightColorLoc;
    int ambientLoc;
    int lightPosLoc;
    int normalLoc;
    int texoLoc;
    int fadeToLoc;
    int vpLoc;
    int lightSpaceMatrixLoc;
    int shadowMapLoc;
    int shadowsEnabledLoc;
    int timeLoc;
    int drunkennessLoc;
    int camPosLoc;
    int shadowMapFarLoc;
    int lightSpaceMatrixFarLoc;
    int cascadeSplitLoc;
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

struct physicsEntity {
    float weight;
    vector3 location;
    vector3 magnitude;
    vector3 newForce;
    vector3 acceleration;
    vector3 applyAccel;
    bool collidingY;
    bool jumping;
    int groundPlane;
    float velocity;
    bool complexGeometry;
    vector3 rot;
};

struct player {
    struct camera camera;
    planeMtx* model; // probably leave blank
    planeMtx* collider; // top and bottom
    planeMtx* colliderO; // original collider for collision response
    int cPlaneCount;
    vector4 controls;
    bool canMove;
    struct physicsEntity pEntity;
    struct playerState pState;
};

struct world {
    planeMtx* planes;
    int planeCount;
};

struct tri {
    vector3 v[3];
    vector3 n[3];
    vector2 t[3];
};

struct triDomMesh {
    tri* trisO;
    tri* tris;
    int count;
};

struct meshedObject {
    struct triDomMesh mesh;
    planeMtx* collider;
    planeMtx* colliderO;
    int cPlaneCount;
    float scale;
    Texture2D texo;
    bool noCull = false;
    char text[100];
    float textRenderDistance;
    bool renderText;
    vector3 relativeTextOffset;
    struct physicsEntity pEntity;
    vector3 offset;

};



void objToQuads(const char* path, meshedObject& mesh, float scale, player& player, bool playerObj);

mtx44 mmult4(const mtx44&, const mtx44&);

vector4 modmmult(const mtx44&, const vector4&);

Matrix ToRaylibMatrix(const mtx44& a);

float dot3(const vector3&, const vector3&);

vector3 cross3(const vector3&, const vector3&);

float len3(const vector3&);

vector3 normalize3(const vector3&);

vector3 extendV2(const vector2&);

vector4 extendV3(const vector3&);

void calculateGon2D(const int n, gonalMtx& out, const bool vertical, const int size);

void spinGon2D(spungonMtx& out, const float size);

float epsCheck(float val, float eps);

vector3 transformToNDC(const mtx44& vp, float x, float y, float z);

float getHypot(float a, float b);

void moveUVs(triDomMesh& mesh, int* coords, int coordcount, float adjustment);

void applyRot(vector3& v, vector3 rot, float xMod, float yMod, float zMod);
void applyCamRot(vector3& v, vector3 camTarget, float xMod, float yMod, float zMod);

bool isPointInCameraRadius(const camera& cam, const vector3& worldPoint, float screenW, float screenH, float radiusPixels);

bool canInteract(const player& player, const vector3& worldPoint, float maxDist, float screenW, float screenH, float radiusPixels, vector3 offset);
