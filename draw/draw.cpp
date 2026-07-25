#include "raylib.h"
#include "rlgl.h"
#include "../util.h"
#include "draw.h"
#include "../camera/camera.h"
#include <cmath>
#include <stdio.h>
#include <iostream>

void Draw3DGPU(const meshedObject& object, const camera& cam, shaderStore& shader, vector4 color, const mtx44* precomputedVP, const Texture2D* shadowTex, bool receiveShadows, const Texture2D* shadowTexFar) {
    const triDomMesh& mesh = object.mesh;
    mtx44 vp;
    if (precomputedVP) {
        vp = *precomputedVP;
    } else {
        mtx44 view = viewMtx44(cam.camPos, cam.camTarget, cam.up);
        mtx44 proj = projMtx44(cam.fov, cam.aspect, 0.1f, 100000000.0f);
        vp = mmult4(proj, view);
    }

    rlDrawRenderBatchActive();

    if (shader.shadowsEnabledLoc >= 0) {
        const int shadowFlag = receiveShadows ? 1 : 0;
        SetShaderValue(shader.shader, shader.shadowsEnabledLoc, &shadowFlag, SHADER_UNIFORM_INT);
    }
    if (shadowTex != nullptr && shadowTex->id != 0 && shader.shadowMapLoc >= 0)
    SetShaderValueTexture(shader.shader, shader.shadowMapLoc, *shadowTex);
    if (shadowTexFar != nullptr && shadowTexFar->id != 0 && shader.shadowMapFarLoc >= 0)
    SetShaderValueTexture(shader.shader, shader.shadowMapFarLoc, *shadowTexFar);
    SetShaderValueTexture(shader.shader, shader.texoLoc, object.texo);
    SetShaderValue(shader.shader, shader.emissiveLoc, &object.emissive, SHADER_UNIFORM_VEC4);
    SetShaderValue(shader.shader, shader.bloomParamsLoc, &object.bloom, SHADER_UNIFORM_VEC2);
    SetShaderValueMatrix(shader.shader, shader.vpLoc, ToRaylibMatrix(vp));
    mtx44 model = buildModelMatrix(object.pEntity.location, object.pEntity.rot, object.offset);
    SetShaderValueMatrix(shader.shader, shader.modelLoc, ToRaylibMatrix(model));

    rlBegin(RL_TRIANGLES);
    rlColor4ub(color.x, color.y, color.z, color.t);
    for (int i = 0; i < mesh.count; i++) {
        rlNormal3f(mesh.trisO[i].n[0].x, mesh.trisO[i].n[0].y, mesh.trisO[i].n[0].z);
        rlTexCoord2f(mesh.trisO[i].t[0].x, mesh.trisO[i].t[0].y);
        rlVertex3f(mesh.trisO[i].v[0].x, mesh.trisO[i].v[0].y, mesh.trisO[i].v[0].z);

        rlNormal3f(mesh.trisO[i].n[1].x, mesh.trisO[i].n[1].y, mesh.trisO[i].n[1].z);
        rlTexCoord2f(mesh.trisO[i].t[1].x, mesh.trisO[i].t[1].y);
        rlVertex3f(mesh.trisO[i].v[1].x, mesh.trisO[i].v[1].y, mesh.trisO[i].v[1].z);

        rlNormal3f(mesh.trisO[i].n[2].x, mesh.trisO[i].n[2].y, mesh.trisO[i].n[2].z);
        rlTexCoord2f(mesh.trisO[i].t[2].x, mesh.trisO[i].t[2].y);
        rlVertex3f(mesh.trisO[i].v[2].x, mesh.trisO[i].v[2].y, mesh.trisO[i].v[2].z);
    }
    rlEnd();
}

void Draw3DDepthGPU(const meshedObject& object, Shader depthShader, int modelLoc) {
    const triDomMesh& mesh = object.mesh;

    rlDrawRenderBatchActive();
    mtx44 model = buildModelMatrix(object.pEntity.location, object.pEntity.rot, object.offset);
    SetShaderValueMatrix(depthShader, modelLoc, ToRaylibMatrix(model));

    rlSetTexture(object.texo.id);
    rlBegin(RL_TRIANGLES);
    for (int i = 0; i < mesh.count; i++) {
        rlTexCoord2f(mesh.trisO[i].t[0].x, mesh.trisO[i].t[0].y);
        rlVertex3f(mesh.trisO[i].v[0].x, mesh.trisO[i].v[0].y, mesh.trisO[i].v[0].z);
        rlTexCoord2f(mesh.trisO[i].t[1].x, mesh.trisO[i].t[1].y);
        rlVertex3f(mesh.trisO[i].v[1].x, mesh.trisO[i].v[1].y, mesh.trisO[i].v[1].z);
        rlTexCoord2f(mesh.trisO[i].t[2].x, mesh.trisO[i].t[2].y);
        rlVertex3f(mesh.trisO[i].v[2].x, mesh.trisO[i].v[2].y, mesh.trisO[i].v[2].z);
    }
    rlEnd();
    rlSetTexture(0);
}

void DrawColliderGPU(int cPlaneCount, const planeMtx* colliders, const camera& cam, shaderStore& shader, vector4 color, const mtx44* precomputedVP) {
    if (debugMode == false) return;
    mtx44 vp;
    if (precomputedVP) {
        vp = *precomputedVP;
    } else {
        mtx44 view = viewMtx44(cam.camPos, cam.camTarget, cam.up);
        mtx44 proj = projMtx44(cam.fov, cam.aspect, 0.1f, 1000000000000000000.0f);
        vp = mmult4(proj, view);
    }

    rlDrawRenderBatchActive();
    SetShaderValueMatrix(shader.shader, shader.vpLoc, ToRaylibMatrix(vp));
    SetShaderValueMatrix(shader.shader, shader.modelLoc, ToRaylibMatrix(identityMatrix()));

    rlBegin(RL_LINES);
    rlColor4ub(color.x, color.y, color.z, color.t);
    for (int i = 0; i < cPlaneCount; i++) {
        for (int j = 0; j < 4; j++) {
            int next = (j + 1) % 4;
            rlVertex3f(colliders[i].m[j][0], colliders[i].m[j][1], colliders[i].m[j][2]);
            rlVertex3f(colliders[i].m[next][0], colliders[i].m[next][1], colliders[i].m[next][2]);
        }
    }
    rlEnd();
}

void DrawPlaneNormalsGPU(int planeCount, const planeMtx* planes, const camera& cam, shaderStore& shader, vector4 color, float length, const mtx44* precomputedVP) {
    if (debugMode == false) return;
    mtx44 vp;
    if (precomputedVP) {
        vp = *precomputedVP;
    } else {
        mtx44 view = viewMtx44(cam.camPos, cam.camTarget, cam.up);
        mtx44 proj = projMtx44(cam.fov, cam.aspect, 0.1f, 1000000000000000000.0f);
        vp = mmult4(proj, view);
    }

    rlDrawRenderBatchActive();
    SetShaderValueMatrix(shader.shader, shader.vpLoc, ToRaylibMatrix(vp));
    SetShaderValueMatrix(shader.shader, shader.modelLoc, ToRaylibMatrix(identityMatrix()));

    rlBegin(RL_LINES);
    rlColor4ub(color.x, color.y, color.z, color.t);
    for (int i = 0; i < planeCount; i++) {
        vector3 p1 = {planes[i].m[0][0], planes[i].m[0][1], planes[i].m[0][2]};
        vector3 p2 = {planes[i].m[1][0], planes[i].m[1][1], planes[i].m[1][2]};
        vector3 p4 = {planes[i].m[3][0], planes[i].m[3][1], planes[i].m[3][2]};

        vector3 center = {
            (planes[i].m[0][0] + planes[i].m[1][0] + planes[i].m[2][0] + planes[i].m[3][0]) * 0.25f,
                (planes[i].m[0][1] + planes[i].m[1][1] + planes[i].m[2][1] + planes[i].m[3][1]) * 0.25f,
                (planes[i].m[0][2] + planes[i].m[1][2] + planes[i].m[2][2] + planes[i].m[3][2]) * 0.25f
        };

        vector3 normal = normalize3(cross3(p2 - p1, p4 - p1));
        vector3 tip = center + normal.fmult(length);

        rlVertex3f(center.x, center.y, center.z);
        rlVertex3f(tip.x, tip.y, tip.z);
    }
    rlEnd();
}

void DrawPlaneGPU(planeMtx plane, camera cam, shaderStore shader, vector4 color, float scale) {
    mtx44 view = viewMtx44(cam.camPos, cam.camTarget, cam.up);
    mtx44 proj = projMtx44(cam.fov, cam.aspect, 0.1f, 1000.0f);
    mtx44 vp = mmult4(proj, view);

    vector3 p0 = {plane.m[0][0], plane.m[0][1], plane.m[0][2]};
    vector3 p1 = {plane.m[1][0], plane.m[1][1], plane.m[1][2]};
    vector3 p2 = {plane.m[2][0], plane.m[2][1], plane.m[2][2]};
    vector3 p3 = {plane.m[3][0], plane.m[3][1], plane.m[3][2]};

    vector3 edge1 = {plane.m[1][0] - plane.m[0][0], plane.m[1][1] - plane.m[0][1], plane.m[1][2] - plane.m[0][2]};
    vector3 edge2 = {plane.m[3][0] - plane.m[0][0], plane.m[3][1] - plane.m[0][1], plane.m[3][2] - plane.m[0][2]};

    float nx = edge1.y * edge2.z - edge1.z * edge2.y;
    float ny = edge1.z * edge2.x - edge1.x * edge2.z;
    float nz = edge1.x * edge2.y - edge1.y * edge2.x;
    float nlen = sqrtf(nx*nx + ny*ny + nz*nz);
    float normal[3] = {nx/nlen, ny/nlen, nz/nlen};

    SetShaderValue(shader.shader, shader.normalLoc, normal, SHADER_UNIFORM_VEC3);

    vector3 ndc1, ndc2, ndc3, ndc4;
    rlBegin(RL_TRIANGLES);
    rlColor4ub(color.x, color.y, color.z, color.t);
    if (CullAndProjectTriangleToNDC(vp, p0, p1, p2, ndc1, ndc2, ndc3)) {

        rlNormal3f(normal[0], normal[1], normal[2]);
        rlVertex3f(ndc1.x, ndc1.y, ndc1.z);
        rlNormal3f(normal[0], normal[1], normal[2]);
        rlVertex3f(ndc2.x, ndc2.y, ndc2.z);
        rlNormal3f(normal[0], normal[1], normal[2]);
        rlVertex3f(ndc3.x, ndc3.y, ndc3.z);
    }
    if (CullAndProjectTriangleToNDC(vp, p0, p2, p3, ndc1, ndc3, ndc4)) {

        rlNormal3f(normal[0], normal[1], normal[2]);
        rlVertex3f(ndc1.x, ndc1.y, ndc1.z);
        rlNormal3f(normal[0], normal[1], normal[2]);
        rlVertex3f(ndc3.x, ndc3.y, ndc3.z);
        rlNormal3f(normal[0], normal[1], normal[2]);
        rlVertex3f(ndc4.x, ndc4.y, ndc4.z);
    }
    rlEnd();
}

void DrawSphereGPU(sphere_ sphere, float n, vector3 centp, camera& cam, float screenW, float screenH) {
    spungonMtx mtxmtx = sphere.spungon_mtx;
    spinGon2D(mtxmtx,n);
    mtx44 world = {};
    vector2 verts[mtxmtx.size][mtxmtx.mtxarr[0].size];

    for (int i = 0; i < mtxmtx.size; i++) {
        for (int j = 0; j < mtxmtx.mtxarr[i].size; j++) {
            world.m[0][0] = 1; world.m[1][1] = 1; world.m[2][2] = 1; world.m[3][3] = 1;

            vector3 object_coords;
            object_coords.x = mtxmtx.mtxarr[i].mtx[j].x + centp.x;
            object_coords.y = mtxmtx.mtxarr[i].mtx[j].y + centp.y;
            object_coords.z = mtxmtx.mtxarr[i].mtx[j].z + centp.z;

            modmmult(world, extendV3(object_coords));
            mtx44 view = viewMtx44(cam.camPos, cam.camTarget, cam.up);
            mtx44 proj = projMtx44(cam.fov, cam.aspect, 0.1f, 1000.0f);

            vector3 screen;
            bool sing_ok = worldToScreen(object_coords, world, view, proj, screenW, screenH, screen);

            if (sing_ok) {

                verts[i][j].x = screen.x;
                verts[i][j].y = screen.y;

            }

        }
    }

    for (int p = 0; p < mtxmtx.size; p ++) {
        for (int k = 0; k < mtxmtx.mtxarr[p].size; k++) {
            if (k == mtxmtx.mtxarr[p].size-1) {
                if (p == mtxmtx.size-1) {
                    DrawLineFancy(verts[k][p].x,verts[k][p].y,verts[0][0].x,verts[0][0].y,BLACK);
                    DrawLineFancy(verts[p][k].x,verts[p][k].y,verts[0][0].x,verts[0][0].y,BLACK);
                }
                else {
                    DrawLineFancy(verts[p][k].x,verts[p][k].y,verts[p][0].x,verts[p][0].y,BLACK);
                    DrawLineFancy(verts[k][p].x,verts[k][p].y,verts[0][p].x,verts[0][p].y,BLACK);

                }
            }
            else {
                DrawLineFancy(verts[p][k].x,verts[p][k].y,verts[p][k+1].x,verts[p][k+1].y,BLACK);
                DrawLineFancy(verts[k][p].x,verts[k][p].y,verts[k+1][p].x,verts[k+1][p].y,BLACK);
            }
        }
    }
}

void DrawLineFancy(float x1, float y1, float x2, float y2, Color color) {
    DrawLineV({(float)x1, (float)y1}, {(float)x2, (float)y2}, color);
}

void DrawTriangleFancy(const triangleMtx& triangle, Color color) {
    DrawLineFancy(triangle.x1, triangle.y1, triangle.x2, triangle.y2, color);
    DrawLineFancy(triangle.x2, triangle.y2, triangle.x3, triangle.y3, color);
    DrawLineFancy(triangle.x3, triangle.y3, triangle.x1, triangle.y1, color);
}

void DrawGon(const int size, gonalMtx& coords) {

    for (int i = 0; i < coords.size; i++) {
        if (i == coords.size-1) {
            std::cout << coords.mtx[i].x << ", " << coords.mtx[i].y << std::endl;
            std::cout << coords.mtx[0].x << ", " << coords.mtx[0].y << std::endl;
            DrawLineFancy(coords.mtx[i].x,coords.mtx[i].y,coords.mtx[0].x,coords.mtx[0].y, BLACK);

        }
        else {
            DrawLineFancy(coords.mtx[i].x,coords.mtx[i].y,coords.mtx[i+1].x,coords.mtx[i+1].y, BLACK);
        }
    }
}

void DrawSphere(sphere_ sphere, camera& cam, float screenW, float screenH) {
    DrawPolyHedron(sphere.spungon_mtx,sphere.size,sphere.location,cam,screenW,screenH);
}

void DrawPolyHedron(spungonMtx mtxmtx, float n, vector3 centp, camera& cam, float screenW, float screenH) {
    spinGon2D(mtxmtx,n);
    mtx44 world = {};
    vector2 verts[mtxmtx.size][mtxmtx.mtxarr[0].size];

    for (int i = 0; i < mtxmtx.size; i++) {
        for (int j = 0; j < mtxmtx.mtxarr[i].size; j++) {
            world.m[0][0] = 1; world.m[1][1] = 1; world.m[2][2] = 1; world.m[3][3] = 1;

            vector3 object_coords;
            object_coords.x = mtxmtx.mtxarr[i].mtx[j].x + centp.x;
            object_coords.y = mtxmtx.mtxarr[i].mtx[j].y + centp.y;
            object_coords.z = mtxmtx.mtxarr[i].mtx[j].z + centp.z;

            modmmult(world, extendV3(object_coords));
            mtx44 view = viewMtx44(cam.camPos, cam.camTarget, cam.up);
            mtx44 proj = projMtx44(cam.fov, cam.aspect, 0.1f, 1000.0f);

            vector3 screen;
            bool sing_ok = worldToScreen(object_coords, world, view, proj, screenW, screenH, screen);

            if (sing_ok) {

                verts[i][j].x = screen.x;
                verts[i][j].y = screen.y;

            }

        }
    }

    for (int p = 0; p < mtxmtx.size; p ++) {
        for (int k = 0; k < mtxmtx.mtxarr[p].size; k++) {
            if (k == mtxmtx.mtxarr[p].size-1) {
                if (p == mtxmtx.size-1) {
                    DrawLineFancy(verts[k][p].x,verts[k][p].y,verts[0][0].x,verts[0][0].y,BLACK);
                    DrawLineFancy(verts[p][k].x,verts[p][k].y,verts[0][0].x,verts[0][0].y,BLACK);
                }
                else {
                    DrawLineFancy(verts[p][k].x,verts[p][k].y,verts[p][0].x,verts[p][0].y,BLACK);
                    DrawLineFancy(verts[k][p].x,verts[k][p].y,verts[0][p].x,verts[0][p].y,BLACK);

                }
            }
            else {
                DrawLineFancy(verts[p][k].x,verts[p][k].y,verts[p][k+1].x,verts[p][k+1].y,BLACK);
                DrawLineFancy(verts[k][p].x,verts[k][p].y,verts[k+1][p].x,verts[k+1][p].y,BLACK);
            }
        }
    }
}

void DrawPlaneFancy(const planeMtx& plane, camera& cam, float screenW, float screenH, Color color, bool drawNormal) {
    mtx44 world = {};
    planeMtx screenCoords;

    for (int k = 0; k < 4; k++) {

        world.m[0][0] = 1; world.m[1][1] = 1; world.m[2][2] = 1; world.m[3][3] = 1;

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

        } else {
            std::cout << "Point is behind camera or outside the view" << std::endl;
        }
    }

    DrawLineFancy(screenCoords.m[0][0], screenCoords.m[0][1], screenCoords.m[1][0], screenCoords.m[1][1], color);
    DrawLineFancy(screenCoords.m[1][0], screenCoords.m[1][1], screenCoords.m[2][0], screenCoords.m[2][1], color);

    DrawLineFancy(screenCoords.m[0][0], screenCoords.m[0][1], screenCoords.m[3][0], screenCoords.m[3][1], color);

    DrawLineFancy(screenCoords.m[2][0], screenCoords.m[2][1], screenCoords.m[3][0], screenCoords.m[3][1], color);

    if (drawNormal) {
        vector3 vt[2];
        world.m[0][0] = 1; world.m[1][1] = 1; world.m[2][2] = 1; world.m[3][3] = 1;

        vector3 centroid = {
            (plane.m[0][0] + plane.m[1][0] + plane.m[2][0] + plane.m[3][0])/4,
                (plane.m[0][1] + plane.m[1][1] + plane.m[2][1] + plane.m[3][1])/4,
                (plane.m[0][2] + plane.m[1][2] + plane.m[2][2] + plane.m[3][2])/4
        };

        vector3 p1 = {plane.m[0][0], plane.m[0][1], plane.m[0][2]};
        vector3 p2 = {plane.m[1][0], plane.m[1][1], plane.m[1][2]};
        vector3 p3 = {plane.m[2][0], plane.m[2][1], plane.m[2][2]};

        vector3 v1 = p2 - p1;
        vector3 v2 = p3 - p1;

        vector3 normal = cross3(v1, v2);

        vector3 repos = centroid - normal.fmult(1 / normal.mag());

        mtx44 view = viewMtx44(cam.camPos, cam.camTarget, cam.up);
        mtx44 proj = projMtx44(cam.fov, cam.aspect, 0.1f, 1000.0f);

        bool nsc1 = worldToScreen(repos, world, view, proj, screenW, screenH, vt[0]);
        bool nsc2 = worldToScreen(centroid, world, view, proj, screenW, screenH, vt[1]);

        if (nsc1 && nsc2) {

            DrawLineFancy(vt[0].x,vt[0].y,vt[1].x,vt[1].y,ORANGE);
        } else {
            std::cout << "Point is behind camera or outside the view" << std::endl;
        }

    }
}

void DrawPyramidFancy(const pyramidMtx& pyramid, camera& cam, float screenW, float screenH, Color color) {

    mtx44 world = {};
    pyramidMtx screenCoords;

    for (int k = 0; k < 4; k++) {

        world.m[0][0] = 1; world.m[1][1] = 1; world.m[2][2] = 1; world.m[3][3] = 1;

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

        } else {
            std::cout << "Point is behind camera or outside the view" << std::endl;
        }
    }

    int faceTri[4][3] = {
        {0, 2, 1},
            {0, 1, 3},
            {1, 2, 3},
            {2, 0, 3},
        };

    float faceZ[4];
    int faceOrder[4] = {0, 1, 2, 3};

    for (int f = 0; f < 4; f++) {
        int a = faceTri[f][0], b = faceTri[f][1], c = faceTri[f][2];
        faceZ[f] = (screenCoords.m[a][2] + screenCoords.m[b][2] + screenCoords.m[c][2]) / 3.0f;
    }

    for (int i = 0; i < 4; i++) {
        for (int j = i + 1; j < 4; j++) {
            if (faceZ[faceOrder[i]] > faceZ[faceOrder[j]]) {
                int tmp = faceOrder[i];
                faceOrder[i] = faceOrder[j];
                faceOrder[j] = tmp;
            }
        }
    }

    rlDisableDepthTest();
    rlDisableColorBlend();
    rlDisableBackfaceCulling();

    rlBegin(RL_TRIANGLES);
    rlSetTexture(pyramid.texture.id);
    rlColor4f(1, 1, 1, 1);

    for (int t = 0; t < 4; t++) {
        int f = faceOrder[t];
        int a = faceTri[f][0];
        int b = faceTri[f][1];
        int c = faceTri[f][2];

        rlTexCoord2f(pyramid.textureArea[f][0], pyramid.textureArea[f][1]);
        rlVertex2f(screenCoords.m[c][0], screenCoords.m[c][1]);

        rlTexCoord2f(pyramid.textureArea[f][2], pyramid.textureArea[f][3]);
        rlVertex2f(screenCoords.m[b][0], screenCoords.m[b][1]);

        rlTexCoord2f(pyramid.textureArea[f][4], pyramid.textureArea[f][5]);
        rlVertex2f(screenCoords.m[a][0], screenCoords.m[a][1]);

    }

    rlEnd();
    rlSetTexture(0);

    rlEnableColorBlend();
    rlEnableDepthTest();
    rlEnableBackfaceCulling();

}

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

Color ColorFromHex(const char *hex) {

    if (hex[0] == '#') hex++;

    unsigned int r = 0,
        g = 0,
        b = 0,
        a = 255;

    if (sscanf(hex, "%02x%02x%02x", &r, &g, &b) == 3) {
        return (Color){(unsigned char) r,
                (unsigned char) g,
                (unsigned char) b,
                (unsigned char) a};
    }

    if (sscanf(hex, "%02x%02x%02x%02x", &r, &g, &b, &a) == 4) {
        return (Color){(unsigned char) r,
                (unsigned char) g,
                (unsigned char) b,
                (unsigned char) a};
    }

    return RAYWHITE;
}
