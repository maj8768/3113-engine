#include "util.h"
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <cstdio>

bool debugMode = false;

buyMenu bs = {
    .state = buyState::START,
    .selection = buySelection::ONE
};

vector4 modmmult(const mtx44& mat, const vector4& vec) {
    vector4 out;
    out.x = mat.m[0][0]*vec.x + mat.m[0][1]*vec.y + mat.m[0][2]*vec.z + mat.m[0][3]*vec.t;
    out.y = mat.m[1][0]*vec.x + mat.m[1][1]*vec.y + mat.m[1][2]*vec.z + mat.m[1][3]*vec.t;
    out.z = mat.m[2][0]*vec.x + mat.m[2][1]*vec.y + mat.m[2][2]*vec.z + mat.m[2][3]*vec.t;
    out.t = mat.m[3][0]*vec.x + mat.m[3][1]*vec.y + mat.m[3][2]*vec.z + mat.m[3][3]*vec.t;
    return out;
}

mtx44 mmult4(const mtx44& matA, const mtx44& matB) {
    mtx44 out{};
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            out.m[i][j] = matA.m[i][0] * matB.m[0][j] +
                matA.m[i][1] * matB.m[1][j] +
                matA.m[i][2] * matB.m[2][j] +
                matA.m[i][3] * matB.m[3][j];
        }
    }
    return out;
}

Matrix ToRaylibMatrix(const mtx44& a) {
    Matrix m;
    m.m0 = a.m[0][0]; m.m4 = a.m[0][1]; m.m8 = a.m[0][2]; m.m12 = a.m[0][3];
    m.m1 = a.m[1][0]; m.m5 = a.m[1][1]; m.m9 = a.m[1][2]; m.m13 = a.m[1][3];
    m.m2 = a.m[2][0]; m.m6 = a.m[2][1]; m.m10 = a.m[2][2]; m.m14 = a.m[2][3];
    m.m3 = a.m[3][0]; m.m7 = a.m[3][1]; m.m11 = a.m[3][2]; m.m15 = a.m[3][3];
    return m;
}

mtx44 identityMatrix() {
    mtx44 m{};
    m.m[0][0] = 1; m.m[1][1] = 1; m.m[2][2] = 1; m.m[3][3] = 1;
    return m;
}

mtx44 buildModelMatrix(vector3 location, vector3 rot, vector3 offset) {
    float cx = cosf(rot.x), sx = sinf(rot.x);
    float cy = cosf(rot.y), sy = sinf(rot.y);
    float cz = cosf(rot.z), sz = sinf(rot.z);

    float r00 = cy*cz, r01 = cz*sy*sx - sz*cx, r02 = cz*sy*cx + sz*sx;
    float r10 = cy*sz, r11 = sz*sy*sx + cz*cx, r12 = sz*sy*cx - cz*sx;
    float r20 = -sy, r21 = cy*sx, r22 = cy*cx;

    mtx44 m{};
    m.m[0][0] = r00; m.m[0][1] = r01; m.m[0][2] = r02;
    m.m[0][3] = r00*offset.x + r01*offset.y + r02*offset.z + location.x;
    m.m[1][0] = r10; m.m[1][1] = r11; m.m[1][2] = r12;
    m.m[1][3] = r10*offset.x + r11*offset.y + r12*offset.z + location.y;
    m.m[2][0] = r20; m.m[2][1] = r21; m.m[2][2] = r22;
    m.m[2][3] = r20*offset.x + r21*offset.y + r22*offset.z + location.z;
    m.m[3][3] = 1;
    return m;
}

vector3 transformToNDC(const mtx44& vp, float x, float y, float z) {
    vector4 v = {x, y, z, 1.0f};
    vector4 clip = modmmult(vp, v);
    float invW = 1.0f / clip.t;
    return vector3(clip.x * invW, clip.y * invW, clip.z * invW);
}

float dot3(const vector3& a, const vector3& b) {
    return a.x*b.x + a.y*b.y + a.z*b.z;
}

vector3 cross3(const vector3& a, const vector3& b) {
    return {a.y*b.z - a.z*b.y,
            a.z*b.x - a.x*b.z,
            a.x*b.y - a.y*b.x};
}

float len3(const vector3& v) {
    return sqrt(v.x*v.x + v.y*v.y + v.z*v.z);
}

vector3 normalize3(const vector3& v) {
    float l = len3(v);
    return {v.x/l, v.y/l, v.z/l};
}

vector3 extendV2(const vector2& v) {
    return {v.x, v.y, 1.0f};
}

vector4 extendV3(const vector3& v) {
    return {v.x, v.y, v.z, 1.0f};
}

void calculateGon2D(const int n, gonalMtx& out, const bool vertical, const int size) {
    out.size = n;
    const float d = 2*M_PI/(float)n;

    for (int i = 0; i < n; i++) {
        out.mtx[i].x = cos(d * i) * size;
        out.mtx[i].y = sin(d * i) * size;
        out.mtx[i].z = 0.f;
    }

    if (!vertical) {
        for (int j = 0; j < n; j++) {
            const int t = out.mtx[j].y;
            out.mtx[j].z = t;
            out.mtx[j].y = 0;
        }
    }
}

void spinGon2D(spungonMtx& out, const float size) {
    int n = out.size;
    const float d = 2*M_PI/(float)n;

    for (int i = 0; i < n; i++) {
        for (int j = 0; j < n; j++) {
            out.mtxarr[i].mtx[j].x = cosf(d * i) * cosf(d * j) * size;
            out.mtxarr[i].mtx[j].y = sinf(d * i) * size;
            out.mtxarr[i].mtx[j].z = 0 - cosf(d * i) * sinf(d * j) * size;
        }
    }

}

float epsCheck(float val, float eps) {
    if (fabs(val) < eps) return 0;
    return val;
}

float getHypot(float a, float b) {
    return sqrtf(powf(a,2) + powf(b,2));
}

void objToQuads(const char* path, meshedObject& mesh, float scale, player& player, bool playerObj) {
    int vertexCount = 0;
    int faceCount = 0;
    FILE *file_ptr;
    char buffer[1024];

    file_ptr = fopen(path, "r");

    if (file_ptr == NULL) {
        perror("Error opening file");
        return;
    }

    while (fgets(buffer, sizeof(buffer), file_ptr) != NULL) {
        if (buffer[0] == 'v' && buffer[1] == ' ') {
            vertexCount++;
        }
        else if (buffer[0] == 'f' && buffer[1] == ' ') {
            faceCount++;
        }
    }

    float (*m)[4][3] = new float[vertexCount][4][3];

    if (playerObj) {
        player.cPlaneCount = faceCount;
        player.collider = new planeMtx[faceCount];
        player.colliderO = new planeMtx[faceCount];

        fseek(file_ptr, 0, SEEK_SET);

        int vertexIndex = 0;
        int faceIndex = 0;

        while (fgets(buffer, sizeof(buffer), file_ptr) != NULL) {
            if (buffer[0] == 'v' && buffer[1] == ' ') {
                char* word = strtok(buffer, " ");
                int coord = 0;

                while ((word = strtok(NULL, " ")) != NULL && coord < 3) {
                    m[vertexIndex][0][coord] = atof(word) * scale;
                    coord++;
                }

                vertexIndex++;
            }
            else if (buffer[0] == 'f' && buffer[1] == ' ') {
                char* word = strtok(buffer, " ");
                int vertInFace = 0;

                while ((word = strtok(NULL, " \n")) != NULL && vertInFace < 4) {
                    int idx = atoi(word) - 1;

                    if (idx >= 0 && idx < vertexCount) {
                        player.collider[faceIndex].m[vertInFace][0] = m[idx][0][0];
                        player.collider[faceIndex].m[vertInFace][1] = m[idx][0][1];
                        player.collider[faceIndex].m[vertInFace][2] = m[idx][0][2];
                    }

                    vertInFace++;
                }
                if (vertInFace == 3) {
                    player.collider[faceIndex].m[3][0] = player.collider[faceIndex].m[2][0];
                    player.collider[faceIndex].m[3][1] = player.collider[faceIndex].m[2][1];
                    player.collider[faceIndex].m[3][2] = player.collider[faceIndex].m[2][2];
                }

                faceIndex++;
            }
        }

        memcpy(player.colliderO, player.collider, faceCount * sizeof(planeMtx));

        delete[] m;
        fclose(file_ptr);
    }
    else {
        mesh.cPlaneCount = faceCount;
        mesh.collider = new planeMtx[faceCount];
        mesh.colliderO = new planeMtx[faceCount];
        fseek(file_ptr, 0, SEEK_SET);

        int vertexIndex = 0;
        int faceIndex = 0;

        while (fgets(buffer, sizeof(buffer), file_ptr) != NULL) {
            if (buffer[0] == 'v' && buffer[1] == ' ') {
                char* word = strtok(buffer, " ");
                int coord = 0;

                while ((word = strtok(NULL, " ")) != NULL && coord < 3) {
                    m[vertexIndex][0][coord] = atof(word) * scale;
                    coord++;
                }

                vertexIndex++;
            }
            else if (buffer[0] == 'f' && buffer[1] == ' ') {
                char* word = strtok(buffer, " ");
                int vertInFace = 0;

                while ((word = strtok(NULL, " \n")) != NULL && vertInFace < 4) {
                    int idx = atoi(word) - 1;

                    if (idx >= 0 && idx < vertexCount) {
                        mesh.collider[faceIndex].m[vertInFace][0] = m[idx][0][0];
                        mesh.collider[faceIndex].m[vertInFace][1] = m[idx][0][1];
                        mesh.collider[faceIndex].m[vertInFace][2] = m[idx][0][2];
                    }

                    vertInFace++;
                }
                if (vertInFace == 3) {
                    mesh.collider[faceIndex].m[3][0] = mesh.collider[faceIndex].m[2][0];
                    mesh.collider[faceIndex].m[3][1] = mesh.collider[faceIndex].m[2][1];
                    mesh.collider[faceIndex].m[3][2] = mesh.collider[faceIndex].m[2][2];
                }

                faceIndex++;
            }
        }

        memcpy(mesh.colliderO, mesh.collider, faceCount * sizeof(planeMtx));

        delete[] m;
        fclose(file_ptr);
    }
}

void moveUVs(triDomMesh& mesh, int* coords, int coordcount, float adjustment) {
    const float scroll = adjustment;

    for (int i = 0; i < coordcount; i++) {

        mesh.tris[coords[i]].t[0].y += scroll;
        mesh.tris[coords[i]].t[1].y += scroll;
        mesh.tris[coords[i]].t[2].y += scroll;

    }
}

void applyCamRot(vector3& v, vector3 camTarget, float xMod, float yMod, float zMod) {
    float cy = cosf(camTarget.x), sy = sinf(camTarget.x);
    float cp = cosf(camTarget.y), sp = sinf(camTarget.y);
    vector3 right = {-sy, 0.f, cy};
    vector3 up = {-cy * sp, cp, -sy * sp};
    vector3 forward = {cp * cy, sp, cp * sy};
    v.x += xMod * right.x + yMod * up.x + zMod * forward.x;
    v.y += xMod * right.y + yMod * up.y + zMod * forward.y;
    v.z += xMod * right.z + yMod * up.z + zMod * forward.z;
}

void applyRot(vector3& v, vector3 rot, float xMod, float yMod, float zMod) {
    vector3 offset = {xMod, yMod, zMod};

    float y = offset.y * cos(rot.x) - offset.z * sin(rot.x);
    float z = offset.z * cos(rot.x) + offset.y * sin(rot.x);
    offset.y = y; offset.z = z;

    float x = offset.x * cos(rot.y) + offset.z * sin(rot.y);
    z = offset.z * cos(rot.y) - offset.x * sin(rot.y);
    offset.x = x; offset.z = z;

    x = offset.x * cos(rot.z) - offset.y * sin(rot.z);
    y = offset.y * cos(rot.z) + offset.x * sin(rot.z);
    offset.x = x; offset.y = y;

    v = v + offset;
}

bool isPointInCameraRadius(const camera& cam, const vector3& worldPoint, float screenW, float screenH, float radiusPixels, vector3 offset)
{
    float cy = cosf(cam.camTarget.x), sy = sinf(cam.camTarget.x);
    float cp = cosf(cam.camTarget.y), sp = sinf(cam.camTarget.y);
    vector3 forward = normalize3({cp * cy, sp, cp * sy});

    vector3 toPoint = worldPoint + offset - cam.camPos;
    float dist = toPoint.mag();
    if (dist < 1e-5f) return true;
    toPoint = normalize3(toPoint);

    float cosAngle = dot3(forward, toPoint);
    if (cosAngle <= 0.f) return false;
    float tanHalfFOV = tanf(cam.fov * 0.5f);
    float tanR = (radiusPixels / (screenH * 0.5f)) * tanHalfFOV;
    float cosThreshold = 1.0f / sqrtf(1.0f + tanR * tanR);
    if (false) {
        std::cout << "playerpos: " << cam.camPos.x << ", " << cam.camPos.y << ", " << cam.camPos.z << std::endl;
        std::cout << "worldPoint: " << worldPoint.x << ", " << worldPoint.y << ", " << worldPoint.z << std::endl;
        std::cout << "camtarget " << cam.camTarget.x << ", " << cam.camTarget.y << std::endl;
        std::cout << "cosAngle: " << cosAngle << " cosThreshold: " << cosThreshold << " dist: " << dist << std::endl;
        std::cout << "toPoint: " << toPoint.x << ", " << toPoint.y << ", " << toPoint.z << std::endl;
        bool f = cosAngle >= cosThreshold;
        std::cout << f << std::endl;
    }
    return cosAngle >= cosThreshold;
}

bool isColliderInCameraRadius(const camera& cam, const planeMtx* collider, int cPlaneCount, float screenW, float screenH, float radiusPixels) {
    if (!collider || cPlaneCount == 0) return false;

    float minX = collider[0].m[0][0], maxX = minX;
    float minY = collider[0].m[0][1], maxY = minY;
    float minZ = collider[0].m[0][2], maxZ = minZ;
    for (int i = 0; i < cPlaneCount; i++)
    for (int v = 0; v < 4; v++) {
        if (collider[i].m[v][0] < minX) minX = collider[i].m[v][0]; else if (collider[i].m[v][0] > maxX) maxX = collider[i].m[v][0];
        if (collider[i].m[v][1] < minY) minY = collider[i].m[v][1]; else if (collider[i].m[v][1] > maxY) maxY = collider[i].m[v][1];
        if (collider[i].m[v][2] < minZ) minZ = collider[i].m[v][2]; else if (collider[i].m[v][2] > maxZ) maxZ = collider[i].m[v][2];
    }

    vector3 center = {(minX+maxX)*0.5f, (minY+maxY)*0.5f, (minZ+maxZ)*0.5f};
    float dx = maxX-minX, dy = maxY-minY, dz = maxZ-minZ;
    float halfDiag = sqrtf(dx*dx + dy*dy + dz*dz) * 0.5f;

    float cy = cosf(cam.camTarget.x), sy = sinf(cam.camTarget.x);
    float cp = cosf(cam.camTarget.y), sp = sinf(cam.camTarget.y);
    vector3 forward = normalize3({cp*cy, sp, cp*sy});

    vector3 toCenter = center - cam.camPos;
    float dist = toCenter.mag();
    if (dist < 1e-5f) return true;
    toCenter = normalize3(toCenter);

    float cosAngle = dot3(forward, toCenter);
    if (cosAngle <= 0.f) return false;

    float tanHalfFOV = tanf(cam.fov * 0.5f);
    float tanPixel = (radiusPixels / (screenH * 0.5f)) * tanHalfFOV;
    float tanSphere = halfDiag / dist;
    float totalTanR = tanPixel + tanSphere;
    float cosThreshold = 1.0f / sqrtf(1.0f + totalTanR * totalTanR);
    return cosAngle >= cosThreshold;
}

bool canInteract(const player& player, const planeMtx* collider, int cPlaneCount, float maxDist, float screenW, float screenH, float radiusPixels) {
    bool c1 = isColliderInCameraRadius(player.camera, collider, cPlaneCount, screenW, screenH, radiusPixels);
    if (!c1) return false;

    float minX = collider[0].m[0][0], maxX = minX;
    float minY = collider[0].m[0][1], maxY = minY;
    float minZ = collider[0].m[0][2], maxZ = minZ;
    for (int i = 0; i < cPlaneCount; i++)
    for (int v = 0; v < 4; v++) {
        if (collider[i].m[v][0] < minX) minX = collider[i].m[v][0]; else if (collider[i].m[v][0] > maxX) maxX = collider[i].m[v][0];
        if (collider[i].m[v][1] < minY) minY = collider[i].m[v][1]; else if (collider[i].m[v][1] > maxY) maxY = collider[i].m[v][1];
        if (collider[i].m[v][2] < minZ) minZ = collider[i].m[v][2]; else if (collider[i].m[v][2] > maxZ) maxZ = collider[i].m[v][2];
    }
    vector3 center = {(minX+maxX)*0.5f, (minY+maxY)*0.5f, (minZ+maxZ)*0.5f};
    return center.dist(player.pEntity.location) < maxDist;
}
