#include "intersect.h"
#include "interpolate.h"
// Suppress warnings in third-party code.
#include <framework/disable_all_warnings.h>
DISABLE_WARNINGS_PUSH()
#include <glm/geometric.hpp>
#include <glm/gtx/component_wise.hpp>
#include <glm/vector_relational.hpp>
DISABLE_WARNINGS_POP()
#include <cmath>
#include <limits>

bool pointInTriangle(const glm::vec3& v0, const glm::vec3& v1, const glm::vec3& v2, const glm::vec3& n, const glm::vec3& p)
{
    glm::vec3 ab = glm::cross(n, v1 - v0);
    glm::vec3 bc = glm::cross(n, v2 - v1);
    glm::vec3 ac = glm::cross(n, v0 - v2);

    int fails = 0;
    if (glm::dot(p - v0, ab) < 0 || glm::dot(p - v1, ab) < 0) {
        fails++;
    }
    if (glm::dot(p - v1, bc) < 0 || glm::dot(p - v2, bc) < 0) {
        fails++;
    }
    if (glm::dot(p - v0, ac) < 0 || glm::dot(p - v2, ac) < 0) {
        fails++;
    }

    // 3 fails are impossible if normals point inwards, therefore they must point outwards (vertices arranged clockwise)
    // that means that point is still inside triangle
    return fails == 0 || fails == 3;
}

bool intersectRayWithPlane(const Plane& plane, Ray& ray)
{
    if ((glm::dot(ray.direction, plane.normal) < 0 && glm::dot(plane.D * plane.normal - ray.origin, plane.normal) < 0)
        || (glm::dot(ray.direction, plane.normal) > 0 && glm::dot(plane.D * plane.normal - ray.origin, plane.normal) > 0)) {
        ray.t = (plane.D - glm::dot(plane.normal, ray.origin)) / glm::dot(plane.normal, ray.direction);
        return true;
    }

    return false;
}

Plane trianglePlane(const glm::vec3& v0, const glm::vec3& v1, const glm::vec3& v2)
{
    glm::vec3 normal = glm::normalize(glm::cross(v0 - v1, v0 - v2));
    return Plane { glm::dot(v0, normal), normal };
}

/// Input: the three vertices of the triangle
/// Output: if intersects then modify the hit parameter ray.t and return true, otherwise return false
bool intersectRayWithTriangle(const glm::vec3& v0, const glm::vec3& v1, const glm::vec3& v2, Ray& ray, HitInfo& hitInfo)
{
    float t = ray.t;
    glm::vec3 normal = glm::normalize(glm::cross(v0 - v1, v0 - v2));
    if (intersectRayWithPlane(trianglePlane(v0, v1, v2), ray)
        && pointInTriangle(v0, v1, v2, normal, ray.origin + ray.direction * ray.t) && ray.t < t) {
        hitInfo.normal = normal;
        hitInfo.barycentricCoord = computeBarycentricCoord(v0, v1, v2, ray.origin + ray.direction * ray.t);
        return true;
    }

    ray.t = t;
    return false;
}

/// Input: a sphere with the following attributes: sphere.radius, sphere.center
/// Output: if intersects then modify the hit parameter ray.t and return true, otherwise return false
bool intersectRayWithShape(const Sphere& sphere, Ray& ray, HitInfo& hitInfo)
{
    float d = 4 * dot(ray.origin - sphere.center, ray.direction) * dot(ray.origin - sphere.center, ray.direction)
        - 4 * dot(ray.direction, ray.direction)
            * (dot(ray.origin - sphere.center, ray.origin - sphere.center) - sphere.radius * sphere.radius);

    if (d < 0) {
        return false;
    }

    float t1 = (-sqrt(d) - 2 * dot(ray.origin - sphere.center, ray.direction)) / (2 * dot(ray.direction, ray.direction));
    float t2 = (sqrt(d) - 2 * dot(ray.origin - sphere.center, ray.direction)) / (2 * dot(ray.direction, ray.direction));
    float t = std::min(t1, t2);

    if (t < ray.t) {
        ray.t = t;
        hitInfo.normal = glm::normalize(ray.origin + ray.direction * ray.t - sphere.center);
        hitInfo.material = sphere.material;
        return true;
    }

    return false;
}

/// Input: an axis-aligned bounding box with the following parameters: minimum coordinates box.lower and maximum coordinates box.upper
/// Output: if intersects then modify the hit parameter ray.t and return true, otherwise return false
bool intersectRayWithShape(const AxisAlignedBox& box, Ray& ray)
{
    bool hit = false;
    float t = ray.t;
    if (ray.direction.x != 0) {
        float t1 = (box.lower.x - ray.origin.x) / ray.direction.x;
        float t2 = (box.upper.x - ray.origin.x) / ray.direction.x;

        if (ray.origin.y + ray.direction.y * t1 >= box.lower.y && ray.origin.y + ray.direction.y * t1 <= box.upper.y
            && ray.origin.z + ray.direction.z * t1 >= box.lower.z && ray.origin.z + ray.direction.z * t1 <= box.upper.z
            && t1 >= 0) {
            hit = true;
            ray.t = std::min(ray.t, t1);
        }

        if (ray.origin.y + ray.direction.y * t2 >= box.lower.y && ray.origin.y + ray.direction.y * t2 <= box.upper.y
            && ray.origin.z + ray.direction.z * t2 >= box.lower.z && ray.origin.z + ray.direction.z * t2 <= box.upper.z
            && t2 >= 0) {
            hit = true;
            ray.t = std::min(ray.t, t2);
        }
    }
    if (ray.direction.y != 0) {
        float t1 = (box.lower.y - ray.origin.y) / ray.direction.y;
        float t2 = (box.upper.y - ray.origin.y) / ray.direction.y;

        if (ray.origin.x + ray.direction.x * t1 >= box.lower.x && ray.origin.x + ray.direction.x * t1 <= box.upper.x
            && ray.origin.z + ray.direction.z * t1 >= box.lower.z && ray.origin.z + ray.direction.z * t1 <= box.upper.z
            && t1 >= 0) {
            hit = true;
            ray.t = std::min(ray.t, t1);
        }

        if (ray.origin.x + ray.direction.x * t2 >= box.lower.x && ray.origin.x + ray.direction.x * t2 <= box.upper.x
            && ray.origin.z + ray.direction.z * t2 >= box.lower.z && ray.origin.z + ray.direction.z * t2 <= box.upper.z
            && t2 >= 0) {
            hit = true;
            ray.t = std::min(ray.t, t2);
        }
    }
    if (ray.direction.z != 0) {
        float t1 = (box.lower.z - ray.origin.z) / ray.direction.z;
        float t2 = (box.upper.z - ray.origin.z) / ray.direction.z;

        if (ray.origin.y + ray.direction.y * t1 >= box.lower.y && ray.origin.y + ray.direction.y * t1 <= box.upper.y
            && ray.origin.x + ray.direction.x * t1 >= box.lower.x && ray.origin.x + ray.direction.x * t1 <= box.upper.x
            && t1 >= 0) {
            hit = true;
            ray.t = std::min(ray.t, t1);
        }

        if (ray.origin.y + ray.direction.y * t2 >= box.lower.y && ray.origin.y + ray.direction.y * t2 <= box.upper.y
            && ray.origin.x + ray.direction.x * t2 >= box.lower.x && ray.origin.x + ray.direction.x * t2 <= box.upper.x
            && t2 >= 0) {
            hit = true;
            ray.t = std::min(ray.t, t2);
        }
    }

    return hit;
}
