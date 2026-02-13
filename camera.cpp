#include "util.h"
#include <cmath>

// World to Screen

bool worldToScreen(vector3& wpos, mtx44& world, mtx44& view, mtx44& projection, float screenW, float screenH, vector2& scpos) {
    mtx44 first = mmult4(projection, view);
    mtx44 second = mmult4(first, world);

    vector4 extendedWpos = { wpos.x, wpos.y, wpos.z, 1.0f};
    vector4 clip = modmmult(second, extendedWpos);

    float epsilon = 10e-5; // from google idk (float epsilon)
    if (clip.t > epsilon) {
        float invT = 1.0f/clip.t;
        float x_ndc = clip.x * invT;
        float y_ndc = clip.y * invT;
        float z_ndc = clip.z * invT;

        if (x_ndc < -1.0f || x_ndc > 1.0f) return false;
        if (y_ndc < -1.0f || y_ndc > 1.0f) return false;
        if (z_ndc < -1.0f || z_ndc > 1.0f) return false;

        scpos.x = (x_ndc + 1.0f) * 0.5f * screenW;
        scpos.y = (1.0f - y_ndc) * 0.5f * screenH;
        return true;
    }
    else {
        return false;
    }
}

mtx44 viewMtx44(const vector3& pos, const vector3& target, const vector3& up) {
  vector3 F = normalize3({ target.x - pos.x, target.y - pos.y, target.z - pos.z });
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
  view.m[2][3] =  dot3(F, pos);

  view.m[3][0] = 0.0f; 
  view.m[3][1] = 0.0f; 
  view.m[3][2] = 0.0f; 
  view.m[3][3] = 1.0f;

  return view;
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
