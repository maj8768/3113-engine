
// current
void DrawPlaneGPU(planeMtx plane, camera cam, shaderStore shader, vector4 color);


// deprecated

void DrawLineFancy(float x1, float y1, float x2, float y2, Color color);
void DrawTriangleFancy(const triangleMtx& triangle, Color color);

void DrawPyramidFancy(const pyramidMtx& pyramid, camera& cam, float screenW, float screenH, Color color);
void DrawPlaneFancy(const planeMtx& plane, camera& cam, float screenW, float screenH, Color color, bool drawNormal);
void DrawGon(const int size, gonalMtx& coords);
void DrawPolyHedron(spungonMtx mtxmtx, float n, vector3 centerpos, camera& cam, float screenW, float screenH);
void DrawSphere(sphere_ sphere, camera& cam, float screenW, float screenH);
void XYZRotatePyramidAboutSelf(pyramidMtx& pyramid, float angleX, float angleY, float angleZ);
void XYZRotatePyramidAboutPoint(pyramidMtx& pyramid, float px, float py, float pz, float ix, float iy, float iz);
void XYZScalePyramidAroundCenter(pyramidMtx& pyramid, float scaleFactor);
// void ZRotatePyramidAboutPoint(pyramidMtx& pyramid, float px, float py, float angle);

void ZRotatePointAround(float cx, float cy, float& x, float& y, float angle);
void ZRotateTriangleAboutSelf(triangleMtx& triangle, float angle);
void ZRotateTriangleAboutPoint(triangleMtx& triangle, float px, float py, float angle);
void XYScaleTriangleAroundCenter(triangleMtx& triangle, float scaleFactor);

Color ColorFromHex(const char *hex);
