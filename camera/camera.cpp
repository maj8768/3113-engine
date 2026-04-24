#include "../util.h"
#include <cmath>

bool worldToScreen(vector3& wpos, mtx44& world, mtx44& view, mtx44& projection, float screenW, float screenH, vector3& scpos) {
    mtx44 first = mmult4(projection, view);
    mtx44 second = mmult4(first, world);

    vector4 extendedWpos = {wpos.x, wpos.y, wpos.z, 1.0f};
    vector4 clip = modmmult(second, extendedWpos);

    float epsilon = 10e-5;
    if (clip.t > epsilon) {
        float invT = 1.0f/clip.t;
        float x_ndc = clip.x * invT;
        float y_ndc = clip.y * invT;
        float z_ndc = clip.z * invT;

        scpos.x = (x_ndc + 1.0f) * 0.5f * screenW;
        scpos.y = (1.0f - y_ndc) * 0.5f * screenH;
        scpos.z = (z_ndc * 0.5f) + 0.5f;
        return true;
    }
    else {
        return false;
    }
}

mtx44 viewMtx44(const vector3& pos, const vector3& target, const vector3& up) {
    float cy = cosf(target.x);
    float sy = sinf(target.x);
    float cp = cosf(target.y);
    float sp = sinf(target.y);

    vector3 F = normalize3({
        cp * cy,
            sp,
            cp * sy
    });

    vector3 R = normalize3(cross3(F, up));
    vector3 U = cross3(R, F);

    mtx44 view{};

    view.m[0][0] = R.x;
    view.m[0][1] = R.y;
    view.m[0][2] = R.z;
    view.m[0][3] = -dot3(R, pos);

    view.m[1][0] = U.x;
    view.m[1][1] = U.y;
    view.m[1][2] = U.z;
    view.m[1][3] = -dot3(U, pos);

    view.m[2][0] = -F.x;
    view.m[2][1] = -F.y;
    view.m[2][2] = -F.z;
    view.m[2][3] = dot3(F, pos);

    view.m[3][0] = 0.0f;
    view.m[3][1] = 0.0f;
    view.m[3][2] = 0.0f;
    view.m[3][3] = 1.0f;

    return view;
}

mtx44 lookAtMtx44(const vector3& eye, const vector3& target, const vector3& up) {
    vector3 F = normalize3(target - eye);
    vector3 R = normalize3(cross3(F, up));
    vector3 U = cross3(R, F);

    mtx44 view{};
    view.m[0][0] = R.x; view.m[0][1] = R.y; view.m[0][2] = R.z; view.m[0][3] = -dot3(R, eye);
    view.m[1][0] = U.x; view.m[1][1] = U.y; view.m[1][2] = U.z; view.m[1][3] = -dot3(U, eye);
    view.m[2][0] = -F.x; view.m[2][1] = -F.y; view.m[2][2] = -F.z; view.m[2][3] = dot3(F, eye);
    view.m[3][0] = 0.f; view.m[3][1] = 0.f; view.m[3][2] = 0.f; view.m[3][3] = 1.f;
    return view;
}

mtx44 orthoMtx44(float l, float r, float b, float t, float zn, float zf) {
    mtx44 m{};
    m.m[0][0] = 2.f / (r - l);
    m.m[1][1] = 2.f / (t - b);
    m.m[2][2] = -2.f / (zf - zn);
    m.m[0][3] = -(r + l) / (r - l);
    m.m[1][3] = -(t + b) / (t - b);
    m.m[2][3] = -(zf + zn) / (zf - zn);
    m.m[3][3] = 1.f;
    return m;
}

mtx44 projMtx44(float fovYRad, float aspect, float zn, float zf) {
    float f = 1.0f / tan(fovYRad * 0.5f);

    float A = (zf + zn) / (zn - zf);
    float B = (2.0f * zf * zn) / (zn - zf);

    mtx44 projection{};
    projection.m[0][0] = f / aspect;
    projection.m[0][1] = 0;
    projection.m[0][2] = 0;
    projection.m[0][3] = 0;

    projection.m[1][0] = 0;
    projection.m[1][1] = f;
    projection.m[1][2] = 0;
    projection.m[1][3] = 0;

    projection.m[2][0] = 0;
    projection.m[2][1] = 0;
    projection.m[2][2] = A;
    projection.m[2][3] = B;

    projection.m[3][0] = 0;
    projection.m[3][1] = 0;
    projection.m[3][2] = -1.0f;
    projection.m[3][3] = 0;

    return projection;
}

bool CullAndProjectTriangleToNDC(const mtx44& vp, const vector3& p0, const vector3& p1, const vector3& p2, vector3& ndc0, vector3& ndc1, vector3& ndc2) {
    auto mul = [&](const vector3& p, float& cx, float& cy, float& cz, float& cw) {
        float x = p.x, y = p.y, z = p.z;
        cx = vp.m[0][0]*x + vp.m[0][1]*y + vp.m[0][2]*z + vp.m[0][3]*1.0f;
        cy = vp.m[1][0]*x + vp.m[1][1]*y + vp.m[1][2]*z + vp.m[1][3]*1.0f;
        cz = vp.m[2][0]*x + vp.m[2][1]*y + vp.m[2][2]*z + vp.m[2][3]*1.0f;
        cw = vp.m[3][0]*x + vp.m[3][1]*y + vp.m[3][2]*z + vp.m[3][3]*1.0f;
    };

    float cx0, cy0, cz0, cw0; mul(p0, cx0, cy0, cz0, cw0);
    float cx1, cy1, cz1, cw1; mul(p1, cx1, cy1, cz1, cw1);
    float cx2, cy2, cz2, cw2; mul(p2, cx2, cy2, cz2, cw2);

    if (cw0 <= 0.0f || cw1 <= 0.0f || cw2 <= 0.0f) {
        return false;
    }

    bool allLeft = (cx0 < -cw0) && (cx1 < -cw1) && (cx2 < -cw2);
    bool allRight = (cx0 > cw0) && (cx1 > cw1) && (cx2 > cw2);
    bool allBottom = (cy0 < -cw0) && (cy1 < -cw1) && (cy2 < -cw2);
    bool allTop = (cy0 > cw0) && (cy1 > cw1) && (cy2 > cw2);
    bool allNear = (cz0 < -cw0) && (cz1 < -cw1) && (cz2 < -cw2);
    bool allFar = (cz0 > cw0) && (cz1 > cw1) && (cz2 > cw2);

    if (allLeft || allRight || allBottom || allTop || allNear || allFar) {
        return false;
    }
    ndc0 = {cx0 / cw0, cy0 / cw0, cz0 / cw0};
    ndc1 = {cx1 / cw1, cy1 / cw1, cz1 / cw1};
    ndc2 = {cx2 / cw2, cy2 / cw2, cz2 / cw2};
    return true;
}
