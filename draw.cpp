#include "raylib.h"
#include "rlgl.h"
#include "draw.h"
#include "camera.h"
#include <cmath>
#include <stdio.h>
#include <iostream>

// 2D

void DrawLineFancy(float x1, float y1, float x2, float y2, Color color) {
    DrawLineV({ (float)x1, (float)y1 }, { (float)x2, (float)y2 }, color);
}


void DrawTriangleFancy(const triangleMtx& triangle, Color color) {
    DrawLineFancy(triangle.x1, triangle.y1, triangle.x2, triangle.y2, color);
    DrawLineFancy(triangle.x2, triangle.y2, triangle.x3, triangle.y3, color);
    DrawLineFancy(triangle.x3, triangle.y3, triangle.x1, triangle.y1, color);
}

// 3D


//worldToScreen(vector3& wpos, mtx44& world, mtx44& view, mtx44& projection, float screenW, float screenH, vector2& scpos)

void DrawPlaneFancy(const planeMtx& plane, camera& cam, float screenW, float screenH, Color color) {
    mtx44 world = {};
    planeMtx screenCoords;

    for (int k = 0; k < 4; k++) {
        // std::cout << k << std::endl;

        world.m[0][0] = 1; world.m[1][1] = 1; world.m[2][2] = 1; world.m[3][3] = 1; // identity mtx

        vector3 object_coords;
        object_coords.x = plane.m[k][0];
        object_coords.y = plane.m[k][1];
        object_coords.z = plane.m[k][2];

        modmmult(world, extendV3(object_coords));
        mtx44 view = viewMtx44(cam.camPos, cam.camTarget, cam.up);
        mtx44 proj = projMtx44(cam.fov, cam.aspect, 0.1f, 1000.0f);

        vector3 screen;
        bool sing_ok = worldToScreen(object_coords, world, view, proj, screenW, screenH, screen);

        if (sing_ok) {
            screenCoords.m[k][0] = screen.x;
            screenCoords.m[k][1] = screen.y;
            screenCoords.m[k][2] = screen.z;
            //std::cout << screen.z << ", " << std::endl;
        } else {
            std::cout << "Point is behind camera or outside the view" << std::endl;
        }
    }

    DrawLineFancy(screenCoords.m[0][0], screenCoords.m[0][1], screenCoords.m[1][0], screenCoords.m[1][1], color);
    DrawLineFancy(screenCoords.m[1][0], screenCoords.m[1][1], screenCoords.m[2][0], screenCoords.m[2][1], color);
    DrawLineFancy(screenCoords.m[2][0], screenCoords.m[2][1], screenCoords.m[0][0], screenCoords.m[0][1], color);
    DrawLineFancy(screenCoords.m[0][0], screenCoords.m[0][1], screenCoords.m[3][0], screenCoords.m[3][1], color);
    DrawLineFancy(screenCoords.m[1][0], screenCoords.m[1][1], screenCoords.m[3][0], screenCoords.m[3][1], color);
    DrawLineFancy(screenCoords.m[2][0], screenCoords.m[2][1], screenCoords.m[3][0], screenCoords.m[3][1], color);
}

void DrawPyramidFancy(const pyramidMtx& pyramid, camera& cam, float screenW, float screenH, Color color) {
    
    mtx44 world = {};
    pyramidMtx screenCoords;

    for (int k = 0; k < 4; k++) {
        // std::cout << k << std::endl;

        world.m[0][0] = 1; world.m[1][1] = 1; world.m[2][2] = 1; world.m[3][3] = 1; // identity mtx

        vector3 object_coords;
        object_coords.x = pyramid.m[k][0];
        object_coords.y = pyramid.m[k][1];
        object_coords.z = pyramid.m[k][2];

        modmmult(world, extendV3(object_coords));
        mtx44 view = viewMtx44(cam.camPos, cam.camTarget, cam.up);
        mtx44 proj = projMtx44(cam.fov, cam.aspect, 0.1f, 1000.0f);

        vector3 screen;
        bool sing_ok = worldToScreen(object_coords, world, view, proj, screenW, screenH, screen);

        if (sing_ok) {
            screenCoords.m[k][0] = screen.x;
            screenCoords.m[k][1] = screen.y;
            screenCoords.m[k][2] = screen.z;
            //std::cout << screen.z << ", " << std::endl;
        } else {
            std::cout << "Point is behind camera or outside the view" << std::endl;
        }
    }
    //std::cout << "next" << std::endl;

    // Draw the base triangles

    //DrawLineFancy(screenCoords.m[0][0], screenCoords.m[0][1], screenCoords.m[1][0], screenCoords.m[1][1], color);
    //DrawLineFancy(screenCoords.m[1][0], screenCoords.m[1][1], screenCoords.m[2][0], screenCoords.m[2][1], color);
    //DrawLineFancy(screenCoords.m[2][0], screenCoords.m[2][1], screenCoords.m[0][0], screenCoords.m[0][1], color);
    //DrawLineFancy(screenCoords.m[0][0], screenCoords.m[0][1], screenCoords.m[3][0], screenCoords.m[3][1], color);
    //DrawLineFancy(screenCoords.m[1][0], screenCoords.m[1][1], screenCoords.m[3][0], screenCoords.m[3][1], color);
    //DrawLineFancy(screenCoords.m[2][0], screenCoords.m[2][1], screenCoords.m[3][0], screenCoords.m[3][1], color);
    // std::cout << "ended" << std::endl;

    // painters (back->front) (no true z bcz no depth layer)

    /*
     * I'm choosing painters because the alternatives require math outside my
     * realm of understanding
    */

    int faceTri[4][3] = {
        {0, 2, 1}, // bottom
        {0, 1, 3}, // side 1
        {1, 2, 3}, // side 2
        {2, 0, 3}, // side 3
    };

    // find barycentric z by averaging z at each coord for each triangle
    float faceZ[4];
    int faceOrder[4] = {0, 1, 2, 3};

    for (int f = 0; f < 4; f++) {
        int a = faceTri[f][0], b = faceTri[f][1], c = faceTri[f][2];
        faceZ[f] = (screenCoords.m[a][2] + screenCoords.m[b][2] + screenCoords.m[c][2]) / 3.0f;
    }

    // sorts faces by z and stores position of farthest face in screencoords first in tri
    for (int i = 0; i < 4; i++) {
        for (int j = i + 1; j < 4; j++) {
            if (faceZ[faceOrder[i]] > faceZ[faceOrder[j]]) {
                int tmp = faceOrder[i];
                faceOrder[i] = faceOrder[j];
                faceOrder[j] = tmp;
            }
        }
    }

    //rlDrawRenderBatchActive();
    rlDisableDepthTest();
    rlDisableColorBlend();
    rlDisableBackfaceCulling();

    rlBegin(RL_TRIANGLES);
    rlSetTexture(pyramid.texture.id);
    rlColor4f(1, 1, 1, 1);

    for (int t = 0; t < 4; t++) { // draws screencoords in order of tri (which stores position of verticies in order of faces far->close)
        int f = faceOrder[t];          // face id
        int a = faceTri[f][0];
        int b = faceTri[f][1];
        int c = faceTri[f][2];

        rlTexCoord2f(pyramid.textureArea[f][0], pyramid.textureArea[f][1]);
        rlVertex2f(screenCoords.m[c][0], screenCoords.m[c][1]);

        rlTexCoord2f(pyramid.textureArea[f][2], pyramid.textureArea[f][3]);
        rlVertex2f(screenCoords.m[b][0], screenCoords.m[b][1]);

        rlTexCoord2f(pyramid.textureArea[f][4], pyramid.textureArea[f][5]);
        rlVertex2f(screenCoords.m[a][0], screenCoords.m[a][1]);

        //std::cout << t << ": " << std::endl;
        //std:: cout << pyramid.textureArea[t][0] << ", " << pyramid.textureArea[t][1] << std::endl;
        //std:: cout << pyramid.textureArea[t][2] << ", " << pyramid.textureArea[t][3] << std::endl;
        //std:: cout << pyramid.textureArea[t][4] << ", " << pyramid.textureArea[t][5] << std::endl;
    }
    //std::cout << "===" << std::endl;

    rlEnd();
    rlSetTexture(0);

    rlEnableColorBlend();
    rlEnableDepthTest();
    rlEnableBackfaceCulling();

    //Vector2 originOffset = {
        //static_cast<float>(gScale.x) / 2.0f,
        // static_cast<float>(gScale.y) / 2.0f
    //    0.f,
    //    0.f
    //};

    //DrawTexturePro(
     //   gTexture,
      //  textureArea,
       // destinationArea,
      //  originOffset,
      //  gAngle,
      //  WHITE
    //);


}


// Transforms

// 2D

void ZRotatePointAboutPoint(float cx, float cy, float& x, float& y, float angle) {
    float px = cos(angle)*(x-cx)-sin(angle)*(y-cy) + cx;
    float py = sin(angle)*(x-cx)+cos(angle)*(y-cy) + cy;

    x = px;
    y = py;
}

void YRotatePointAboutPoint(float cx, float cz, float& y, float& z, float angle) {
    float py = cos(angle)*(y-cx)-sin(angle)*(z-cz) + cx;
    float pz = sin(angle)*(y-cx)+cos(angle)*(z-cz) + cz;

    y = py;
    z = pz;
}

void XRotatePointAboutPoint(float cy, float cz, float& x, float& z, float angle) {
    float px = cos(angle)*(x-cy)-sin(angle)*(z-cz) + cy;
    float pz = sin(angle)*(x-cy)+cos(angle)*(z-cz) + cz;

    x = px;
    z = pz;
}

void ZRotateTriangleAboutSelf(triangleMtx& triangle, float angle) {

    float cx = (triangle.x1 + triangle.x2 + triangle.x3) / 3.0f;
    float cy = (triangle.y1 + triangle.y2 + triangle.y3) / 3.0f;

    ZRotatePointAboutPoint(cx, cy, triangle.x1, triangle.y1, angle);
    ZRotatePointAboutPoint(cx, cy, triangle.x2, triangle.y2, angle);
    ZRotatePointAboutPoint(cx, cy, triangle.x3, triangle.y3, angle);
}

void ZRotateTriangleAboutPoint(triangleMtx& triangle, float px, float py, float angle) {
    ZRotatePointAboutPoint(px, py, triangle.x1, triangle.y1, angle);
    ZRotatePointAboutPoint(px, py, triangle.x2, triangle.y2, angle);
    ZRotatePointAboutPoint(px, py, triangle.x3, triangle.y3, angle);
}

void XYScaleTriangleAroundCenter(triangleMtx& triangle, float scaleFactor) {
    
    float cx = (triangle.x1 + triangle.x2 + triangle.x3) / 3.0f;
    float cy = (triangle.y1 + triangle.y2 + triangle.y3) / 3.0f;

    triangle.x1 = cx + (triangle.x1 - cx) * scaleFactor;
    triangle.y1 = cy + (triangle.y1 - cy) * scaleFactor;

    triangle.x2 = cx + (triangle.x2 - cx) * scaleFactor;
    triangle.y2 = cy + (triangle.y2 - cy) * scaleFactor;

    triangle.x3 = cx + (triangle.x3 - cx) * scaleFactor;
    triangle.y3 = cy + (triangle.y3 - cy) * scaleFactor;
}

// 3D (this is fake, uses made up z from raylib)

// need to redo / repurpose into world space transforms

// void XYZScalePyramidAroundCenter(pyramidMtx& pyramid, float scaleFactor) {
    
//     float cx = (pyramid.x1 + pyramid.x2 + pyramid.x3 + pyramid.x4) / 4.0f;
//     float cy = (pyramid.y1 + pyramid.y2 + pyramid.y3 + pyramid.y4) / 4.0f;
//     float cz = (pyramid.z1 + pyramid.z2 + pyramid.z3 + pyramid.z4) / 4.0f;

//     pyramid.x1 = cx + (pyramid.x1 - cx) * scaleFactor;
//     pyramid.y1 = cy + (pyramid.y1 - cy) * scaleFactor;
//     pyramid.z1 = cz + (pyramid.z1 - cz) * scaleFactor;

//     pyramid.x2 = cx + (pyramid.x2 - cx) * scaleFactor;
//     pyramid.y2 = cy + (pyramid.y2 - cy) * scaleFactor;
//     pyramid.z2 = cz + (pyramid.z2 - cz) * scaleFactor;

//     pyramid.x3 = cx + (pyramid.x3 - cx) * scaleFactor;
//     pyramid.y3 = cy + (pyramid.y3 - cy) * scaleFactor;
//     pyramid.z3 = cz + (pyramid.z3 - cz) * scaleFactor;

//     pyramid.x4 = cx + (pyramid.x4 - cx) * scaleFactor;
//     pyramid.y4 = cy + (pyramid.y4 - cy) * scaleFactor;
//     pyramid.z4 = cz + (pyramid.z4 - cz) * scaleFactor;
// }

// void XYZRotatePyramidAboutSelf(pyramidMtx& pyramid, float angleX, float angleY, float angleZ) {
//     float cx = (pyramid.x1 + pyramid.x2 + pyramid.x3 + pyramid.x4) / 4.0f;
//     float cy = (pyramid.y1 + pyramid.y2 + pyramid.y3 + pyramid.y4) / 4.0f;
//     float cz = (pyramid.z1 + pyramid.z2 + pyramid.z3 + pyramid.z4) / 4.0f;

//     // Z rotation: rotate (x, y) about (cx, cy)
//     ZRotatePointAboutPoint(cx, cy, pyramid.x1, pyramid.y1, angleZ);
//     ZRotatePointAboutPoint(cx, cy, pyramid.x2, pyramid.y2, angleZ);
//     ZRotatePointAboutPoint(cx, cy, pyramid.x3, pyramid.y3, angleZ);
//     ZRotatePointAboutPoint(cx, cy, pyramid.x4, pyramid.y4, angleZ);

//     // Y rotation: rotate (x, z) about (cx, cz)
//     ZRotatePointAboutPoint(cx, cz, pyramid.x1, pyramid.z1, angleY);
//     ZRotatePointAboutPoint(cx, cz, pyramid.x2, pyramid.z2, angleY);
//     ZRotatePointAboutPoint(cx, cz, pyramid.x3, pyramid.z3, angleY);
//     ZRotatePointAboutPoint(cx, cz, pyramid.x4, pyramid.z4, angleY);

//     // X rotation: rotate (y, z) about (cy, cz)
//     ZRotatePointAboutPoint(cy, cz, pyramid.y1, pyramid.z1, angleX);
//     ZRotatePointAboutPoint(cy, cz, pyramid.y2, pyramid.z2, angleX);
//     ZRotatePointAboutPoint(cy, cz, pyramid.y3, pyramid.z3, angleX);
//     ZRotatePointAboutPoint(cy, cz, pyramid.y4, pyramid.z4, angleX);
// }

// void XYZRotatePyramidAboutPoint(pyramidMtx& pyramid, float px, float py, float pz, float ix, float iy, float iz) {
//     // Rotate the pyramid about a point (px, py, pz)

//     float angleX = atan2(iy - py, iz - pz);
//     float angleY = atan2(ix - px, iz - pz);
//     float angleZ = atan2(iy - py, ix - px);

//     XYZRotatePyramidAboutSelf(pyramid, angleX, angleY, angleZ);
// }

// textures

Color ColorFromHex(const char *hex) {
    // Skip leading '#', if present
    if (hex[0] == '#') hex++;

    // Default alpha = 255 (opaque)
    unsigned int r = 0, 
                 g = 0, 
                 b = 0, 
                 a = 255;

    // 6‑digit form: RRGGBB
    if (sscanf(hex, "%02x%02x%02x", &r, &g, &b) == 3) {
        return (Color){ (unsigned char) r,
                        (unsigned char) g,
                        (unsigned char) b,
                        (unsigned char) a };
    }

    // 8‑digit form: RRGGBBAA
    if (sscanf(hex, "%02x%02x%02x%02x", &r, &g, &b, &a) == 4) {
        return (Color){ (unsigned char) r,
                        (unsigned char) g,
                        (unsigned char) b,
                        (unsigned char) a };
    }

    // Fallback – return white so you notice something went wrong
    return RAYWHITE;
}