#include "item_body.h"
#include "raymath.h"

#include <algorithm>
#include <cmath>
#include <limits>

namespace {

constexpr float SKIN = 0.002f;
constexpr float SLOP = 0.0005f;
constexpr float FRICTION = 0.65f;
constexpr int MAX_CONTACTS = 32;
constexpr int SOLVER_PASSES = 6;

float dot(vector3 a, vector3 b) { return a.x*b.x + a.y*b.y + a.z*b.z; }
vector3 cross(vector3 a, vector3 b) {
    return {a.y*b.z - a.z*b.y, a.z*b.x - a.x*b.z, a.x*b.y - a.y*b.x};
}
float lengthSquared(vector3 v) { return dot(v, v); }
float component(vector3 v, int i) { return i == 0 ? v.x : i == 1 ? v.y : v.z; }
vector3 vertex(const planeMtx& p, int i) { return {p.m[i][0], p.m[i][1], p.m[i][2]}; }

struct boxPose {
    vector3 center;
    vector3 axis[3];
    vector3 extent; // world AABB half size
};

boxPose pose(const itemBody& b, vector3 origin) {
    Matrix m = QuaternionToMatrix(b.orientation);
    boxPose p;
    p.axis[0] = {m.m0, m.m1, m.m2};
    p.axis[1] = {m.m4, m.m5, m.m6};
    p.axis[2] = {m.m8, m.m9, m.m10};
    p.center = origin;
    for (int i = 0; i < 3; ++i) p.center += p.axis[i].fmult(component(b.localCenter, i));
    p.extent = {
        fabsf(m.m0)*b.halfSize.x + fabsf(m.m4)*b.halfSize.y + fabsf(m.m8)*b.halfSize.z,
        fabsf(m.m1)*b.halfSize.x + fabsf(m.m5)*b.halfSize.y + fabsf(m.m9)*b.halfSize.z,
        fabsf(m.m2)*b.halfSize.x + fabsf(m.m6)*b.halfSize.y + fabsf(m.m10)*b.halfSize.z};
    return p;
}

vector3 eulerRotation(Quaternion q) {
    Matrix m = QuaternionToMatrix(q);
    // At +/-90 degrees of yaw, x and z are coupled. Pick z=0 rather than
    // evaluating two near-zero atan2 arguments and snapping the rendered pose.
    if (fabsf(m.m2) > 0.999999f)
        return {atan2f(-m.m9, m.m5), copysignf(1.57079632679f, -m.m2), 0.f};
    return {atan2f(m.m6, m.m10), asinf(-m.m2), atan2f(m.m1, m.m0)};
}

vector3 inverseInertia(const itemBody& b, const boxPose& p, vector3 v) {
    vector3 result;
    for (int i = 0; i < 3; ++i)
        result += p.axis[i].fmult(dot(v, p.axis[i])*component(b.inverseInertia, i));
    return result;
}

struct contact {
    vector3 point;
    vector3 normal;
    float impulse = 0.f;
    vector3 frictionImpulse;
};

// SAT against a finite quad, including edge/edge axes so a box can fall off a
// ledge rather than colliding with an infinite extension of the floor.
bool overlaps(const itemBody& b, const boxPose& p, const vector3 q[4],
              vector3& normal, float& depth) {
    vector3 axes[16];
    int count = 0;
    axes[count++] = cross(q[1] - q[0], q[3] - q[0]);
    if (lengthSquared(axes[0]) < 1e-12f) return false;
    for (int i = 0; i < 3; ++i) axes[count++] = p.axis[i];
    // Rectangles need only two edge directions. Also support triangular faces
    // (the OBJ loader repeats their last vertex) and other convex quads.
    vector3 edges[4];
    int edgeCount = 0;
    for (int e = 0; e < 4; ++e) {
        vector3 edge = q[(e + 1)%4] - q[e];
        float e2 = lengthSquared(edge);
        if (e2 < 1e-12f) continue;
        bool duplicate = false;
        for (int i = 0; i < edgeCount; ++i)
            if (lengthSquared(cross(edge, edges[i])) < 1e-8f*e2*lengthSquared(edges[i])) duplicate = true;
        if (duplicate) continue;
        edges[edgeCount++] = edge;
        for (int i = 0; i < 3; ++i) axes[count++] = cross(edge, p.axis[i]);
    }
    depth = std::numeric_limits<float>::max();
    for (int i = 0; i < count; ++i) {
        float n2 = lengthSquared(axes[i]);
        if (n2 < 1e-10f) continue;
        vector3 a = axes[i].fmult(1.f/sqrtf(n2));
        float r = fabsf(dot(a, p.axis[0]))*b.halfSize.x
                + fabsf(dot(a, p.axis[1]))*b.halfSize.y
                + fabsf(dot(a, p.axis[2]))*b.halfSize.z;
        float lo = dot(q[0] - p.center, a), hi = lo;
        for (int k = 1; k < 4; ++k) {
            float d = dot(q[k] - p.center, a);
            lo = fminf(lo, d); hi = fmaxf(hi, d);
        }
        float plus = hi + r, minus = r - lo;
        if (plus < -SKIN || minus < -SKIN) return false;
        float overlap = fminf(plus, minus);
        if (overlap < depth) {
            depth = overlap;
            normal = a.fmult(plus < minus ? 1.f : -1.f);
        }
    }
    return true;
}

// Clip the world quad to the six box planes. This gives a small contact patch,
// including when a floor corner is under a box but none of its eight corners
// lies over the floor. The patch shrinks naturally to an edge or a point.
int contactPatch(const itemBody& b, const boxPose& p, const vector3 q[4], vector3 out[16]) {
    vector3 a[16], scratch[16];
    std::copy(q, q + 4, a);
    int count = 4;
    for (int axis = 0; axis < 3 && count; ++axis) {
        for (int sign = -1; sign <= 1 && count; sign += 2) {
            vector3 n = p.axis[axis].fmult((float)sign);
            float limit = component(b.halfSize, axis) + SKIN;
            int next = 0;
            vector3 prev = a[count - 1];
            float pd = dot(prev - p.center, n) - limit;
            for (int i = 0; i < count; ++i) {
                float d = dot(a[i] - p.center, n) - limit;
                if ((d <= 0.f) != (pd <= 0.f))
                    scratch[next++] = prev + (a[i] - prev).fmult(pd/(pd - d));
                if (d <= 0.f) scratch[next++] = a[i];
                prev = a[i]; pd = d;
            }
            count = next;
            std::copy(scratch, scratch + count, a);
        }
    }
    std::copy(a, a + count, out);
    return count;
}

int findContacts(const itemBody& b, boxPose& p, const world& scene,
                 contact contacts[MAX_CONTACTS], int& supportSegment, int& supportPlane) {
    int count = 0;
    supportSegment = supportPlane = -1;
    for (int s = 0; s < scene.segmentCount; ++s) {
        const worldSegment& seg = scene.segments[s];
        for (int f = 0; f < seg.count; ++f) {
            vector3 q[4];
            vector3 lo = vertex(seg.planes[f], 0), hi = lo;
            for (int i = 0; i < 4; ++i) {
                q[i] = vertex(seg.planes[f], i);
                lo = {fminf(lo.x, q[i].x), fminf(lo.y, q[i].y), fminf(lo.z, q[i].z)};
                hi = {fmaxf(hi.x, q[i].x), fmaxf(hi.y, q[i].y), fmaxf(hi.z, q[i].z)};
            }
            if (p.center.x + p.extent.x + SKIN < lo.x || p.center.x - p.extent.x - SKIN > hi.x ||
                p.center.y + p.extent.y + SKIN < lo.y || p.center.y - p.extent.y - SKIN > hi.y ||
                p.center.z + p.extent.z + SKIN < lo.z || p.center.z - p.extent.z - SKIN > hi.z) continue;
            vector3 normal;
            float depth;
            if (!overlaps(b, p, q, normal, depth)) continue;
            vector3 points[16];
            int n = contactPatch(b, p, q, points);
            if (!n) continue;
            for (int i = 0; i < n && count < MAX_CONTACTS; ++i)
                contacts[count++] = {points[i], normal, 0.f, {}};
            // Correct penetration once per patch, not once per vertex.
            p.center += normal.fmult(fmaxf(depth - SLOP, 0.f)*0.8f);
            if (normal.y > 0.5f) { supportSegment = s; supportPlane = f; }
        }
    }
    return count;
}

void impulse(itemBody& b, physicsEntity& e, const boxPose& p, vector3 r, vector3 j) {
    e.magnitude += j.fmult(b.inverseMass);
    b.angularVelocity += inverseInertia(b, p, cross(r, j));
}

float effectiveMass(const itemBody& b, const boxPose& p, vector3 r, vector3 n) {
    return b.inverseMass + dot(n, cross(inverseInertia(b, p, cross(r, n)), r));
}

void solve(itemBody& b, physicsEntity& e, const boxPose& p, contact* contacts, int count) {
    for (int pass = 0; pass < SOLVER_PASSES; ++pass) {
        for (int i = 0; i < count; ++i) {
            contact& c = contacts[i];
            vector3 r = c.point - p.center;
            vector3 v = e.magnitude + cross(b.angularVelocity, r);
            float old = c.impulse;
            c.impulse = fmaxf(0.f, old - dot(v, c.normal)/effectiveMass(b, p, r, c.normal));
            impulse(b, e, p, r, c.normal.fmult(c.impulse - old));

            v = e.magnitude + cross(b.angularVelocity, r);
            vector3 tangent = v - c.normal.fmult(dot(v, c.normal));
            float t2 = lengthSquared(tangent);
            if (t2 < 1e-12f) continue;
            tangent = tangent.fmult(1.f/sqrtf(t2));
            vector3 friction = c.frictionImpulse - tangent.fmult(dot(v, tangent)/effectiveMass(b, p, r, tangent));
            float limit = FRICTION*c.impulse;
            float f2 = lengthSquared(friction);
            if (f2 > limit*limit) friction = friction.fmult(limit/sqrtf(f2));
            impulse(b, e, p, r, friction - c.frictionImpulse);
            c.frictionImpulse = friction;
        }
    }
}

float crossXZ(vector3 a, vector3 b, vector3 c) {
    return (b.x-a.x)*(c.z-a.z) - (b.z-a.z)*(c.x-a.x);
}

// Only sleep if the gravity line is inside the contact support polygon. A quiet
// object overhanging a ledge must keep tipping, rather than freeze on one corner.
bool supported(vector3 center, const contact* contacts, int count) {
    vector3 points[MAX_CONTACTS], hull[2*MAX_CONTACTS];
    int n = 0;
    for (int i = 0; i < count; ++i)
        if (contacts[i].normal.y > 0.5f) points[n++] = contacts[i].point;
    if (n < 3) return false;
    std::sort(points, points + n, [](vector3 a, vector3 b) {
        return a.x < b.x || (a.x == b.x && a.z < b.z);
    });
    int k = 0;
    for (int i = 0; i < n; ++i) {
        while (k >= 2 && crossXZ(hull[k-2], hull[k-1], points[i]) <= 0.f) --k;
        hull[k++] = points[i];
    }
    int lower = k + 1;
    for (int i = n - 2; i >= 0; --i) {
        while (k >= lower && crossXZ(hull[k-2], hull[k-1], points[i]) <= 0.f) --k;
        hull[k++] = points[i];
    }
    if (k < 4) return false;
    for (int i = 0; i < k - 1; ++i)
        if (crossXZ(hull[i], hull[i+1], center) < -1e-6f) return false;
    return true;
}

bool supportUnchanged(const itemBody& b, const world& scene) {
    if (b.supportSegment < 0 || b.supportSegment >= scene.segmentCount) return false;
    const worldSegment& seg = scene.segments[b.supportSegment];
    if (b.supportPlane < 0 || b.supportPlane >= seg.count) return false;
    for (int i = 0; i < 4; ++i)
        if (lengthSquared(vertex(seg.planes[b.supportPlane], i) - b.supportVertices[i]) > 1e-12f) return false;
    return true;
}

} // namespace

void initializeItemBody(itemBody& b, const meshedObject& object) {
    b = {};
    vector3 lo = {1e30f, 1e30f, 1e30f}, hi = {-1e30f, -1e30f, -1e30f};
    auto include = [&](vector3 v) {
        lo = {fminf(lo.x, v.x), fminf(lo.y, v.y), fminf(lo.z, v.z)};
        hi = {fmaxf(hi.x, v.x), fmaxf(hi.y, v.y), fmaxf(hi.z, v.z)};
    };
    if (object.colliderO && object.cPlaneCount > 0) {
        for (int i = 0; i < object.cPlaneCount; ++i)
            for (int j = 0; j < 4; ++j) include(vertex(object.colliderO[i], j));
    } else if (object.mesh.trisO && object.mesh.count > 0) {
        for (int i = 0; i < object.mesh.count; ++i)
            for (int j = 0; j < 3; ++j) include(object.mesh.trisO[i].v[j] + object.offset);
    } else {
        lo = {-0.05f, -0.05f, -0.05f}; hi = {0.05f, 0.05f, 0.05f};
    }
    b.localCenter = (lo + hi).fmult(0.5f);
    b.halfSize = (hi - lo).fmult(0.5f);
    b.halfSize = {fmaxf(b.halfSize.x, 0.01f), fmaxf(b.halfSize.y, 0.01f), fmaxf(b.halfSize.z, 0.01f)};
    b.radius = sqrtf(lengthSquared(b.halfSize));
    b.inverseMass = 1.f/fmaxf(object.pEntity.weight, 0.001f);
    vector3 h2 = b.halfSize*b.halfSize;
    b.inverseInertia = {3.f*b.inverseMass/(h2.y+h2.z),
                        3.f*b.inverseMass/(h2.x+h2.z),
                        3.f*b.inverseMass/(h2.x+h2.y)};
    vector3 r = object.pEntity.rot;
    b.orientation = b.previousOrientation = QuaternionFromEuler(r.x, r.y, r.z);
}

void resetItemBody(itemBody& b, physicsEntity& e) {
    b.orientation = b.previousOrientation = QuaternionFromEuler(e.rot.x, e.rot.y, e.rot.z);
    b.angularVelocity = {};
    b.quietTime = 0.f;
    b.sleeping = false;
    b.supportSegment = b.supportPlane = -1;
    e.magnitude = {};
    e.newForce = {};
    e.collidingY = false;
}

void releaseItemBody(itemBody& b, physicsEntity& e, vector3 direction) {
    resetItemBody(b, e);
    float d2 = lengthSquared(direction);
    if (d2 > 1e-12f) e.magnitude = direction.fmult(0.25f/sqrtf(d2));
}

void stepItemBody(itemBody& b, physicsEntity& e, float dt, const world& scene) {
    b.previousOrientation = b.orientation;
    if (dt <= 0.f) return;
    if (b.sleeping && lengthSquared(e.magnitude) == 0.f && lengthSquared(b.angularVelocity) == 0.f &&
        lengthSquared(e.newForce) == 0.f && supportUnchanged(b, scene)) return;
    b.sleeping = false;

    vector3 acceleration = e.acceleration*e.applyAccel + e.newForce.fmult(b.inverseMass);
    e.newForce = {};
    // Bounded adaptive substeps protect thin items without increasing the world's
    // fixed tick rate. Ordinary slow movement takes just one substep.
    float travel = (fabsf(e.magnitude.x) + fabsf(e.magnitude.y) + fabsf(e.magnitude.z)
                  + sqrtf(lengthSquared(b.angularVelocity))*b.radius
                  + sqrtf(lengthSquared(acceleration))*dt)*dt;
    float stride = fmaxf(0.5f*fminf(b.halfSize.x, fminf(b.halfSize.y, b.halfSize.z)), 0.01f);
    int steps = std::max(1, std::min(8, (int)ceilf(travel/stride)));
    float h = dt/(float)steps;
    for (int sub = 0; sub < steps; ++sub) {
        boxPose p = pose(b, e.location);
        e.magnitude += acceleration.fmult(h);
        p.center += e.magnitude.fmult(h);
        Quaternion spin = {b.angularVelocity.x, b.angularVelocity.y, b.angularVelocity.z, 0.f};
        Quaternion dq = QuaternionMultiply(spin, b.orientation);
        b.orientation = QuaternionNormalize({b.orientation.x + dq.x*h*0.5f,
                                              b.orientation.y + dq.y*h*0.5f,
                                              b.orientation.z + dq.z*h*0.5f,
                                              b.orientation.w + dq.w*h*0.5f});
        // Rotate around the center of mass, even for models whose origin is at a tip.
        boxPose rotated = pose(b, {});
        e.location = p.center - rotated.center;
        p = pose(b, e.location);
        contact contacts[MAX_CONTACTS];
        int segment, plane;
        int count = findContacts(b, p, scene, contacts, segment, plane);
        solve(b, e, p, contacts, count);
        b.angularVelocity = b.angularVelocity.fmult(1.f/(1.f + h*0.05f));
        e.location = p.center - rotated.center;
        e.collidingY = segment >= 0;
        if (e.collidingY && lengthSquared(e.magnitude) < 0.035f*0.035f &&
            lengthSquared(b.angularVelocity)*b.radius*b.radius < 0.035f*0.035f && supported(p.center, contacts, count)) {
            b.quietTime += h;
            if (b.quietTime >= 0.5f) {
                b.sleeping = true;
                b.angularVelocity = {};
                e.magnitude = {};
                b.supportSegment = segment; b.supportPlane = plane;
                for (int i = 0; i < 4; ++i) b.supportVertices[i] = vertex(scene.segments[segment].planes[plane], i);
                break;
            }
        } else b.quietTime = 0.f;
    }
    e.rot = eulerRotation(b.orientation);
}

vector3 itemBodyRenderRotation(const itemBody& b, float alpha) {
    Quaternion to = b.orientation;
    Quaternion from = b.previousOrientation;
    if (from.x*to.x + from.y*to.y + from.z*to.z + from.w*to.w < 0.f)
        to = {-to.x, -to.y, -to.z, -to.w};
    return eulerRotation(QuaternionNlerp(from, to, alpha));
}
