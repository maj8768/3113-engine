// World to Screen

bool worldToScreen(vector3&, mtx44&, mtx44&, mtx44&, float, float, vector2&);

mtx44 viewMtx44(const vector3&, const vector3&, const vector3&);

mtx44 projMtx44(float, float, float, float);