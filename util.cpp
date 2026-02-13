#include "util.h"
#include <cmath>

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
            out.m[i][j] =   matA.m[i][0] * matB.m[0][j] +
                            matA.m[i][1] * matB.m[1][j] +
                            matA.m[i][2] * matB.m[2][j] +
                            matA.m[i][3] * matB.m[3][j]; 
        }
    }
    return out;
}

float dot3(const vector3& a, const vector3& b) {
  return a.x*b.x + a.y*b.y + a.z*b.z;
}

vector3 cross3(const vector3& a, const vector3& b) {
  return { a.y*b.z - a.z*b.y,
           a.z*b.x - a.x*b.z,
           a.x*b.y - a.y*b.x };
}

float len3(const vector3& v) {
  return sqrt(v.x*v.x + v.y*v.y + v.z*v.z);
}

vector3 normalize3(const vector3& v) {
  float l = len3(v);
  return { v.x/l, v.y/l, v.z/l };
}

vector4 extendV3(const vector3& v) {
  return { v.x, v.y, v.z, 1.0f };
}