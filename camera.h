// World to Screen

bool worldToScreen(vector3&, mtx44&, mtx44&, mtx44&, float, float, vector3&);

mtx44 viewMtx44(const vector3&, const vector3&, const vector3&);

mtx44 projMtx44(float, float, float, float);

bool CullAndProjectTriangleToNDC(const mtx44& vp, const vector3& p0, const vector3& p1, const vector3& p2, vector3& ndc0, vector3& ndc1, vector3& ndc2);
