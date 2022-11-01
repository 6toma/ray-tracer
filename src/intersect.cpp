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

bool pointInTriangle(
    const glm::vec3& v0, const glm::vec3& v1, const glm::vec3& v2, const glm::vec3& n, const glm::vec3& p)
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
        || (glm::dot(ray.direction, plane.normal) > 0
            && glm::dot(plane.D * plane.normal - ray.origin, plane.normal) > 0)) {
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
    // float t = ray.t;
    // glm::vec3 normal = glm::normalize(glm::cross(v0 - v1, v0 - v2));
    // if (intersectRayWithPlane(trianglePlane(v0, v1, v2), ray)
    //     && pointInTriangle(v0, v1, v2, normal, ray.origin + ray.direction * ray.t) && ray.t < t) {
    //     hitInfo.barycentricCoord = computeBarycentricCoord(v0, v1, v2, ray.origin + ray.direction * ray.t);
    //     hitInfo.normal = normal;
    //     return true;
    // }

    // ray.t = t;
    // return false;

    // Moeller-Trumbore intersection
    const float EPSILON = 0.0000001;
    glm::vec3 edge1 = v1 - v0;
    glm::vec3 edge2 = v2 - v0;
    glm::vec3 h = glm::cross(ray.direction, edge2);
    float a = glm::dot(edge1, h);
    if (a > -EPSILON && a < EPSILON)
        return false; // This ray is parallel to this triangle.
    glm::vec3 s = ray.origin - v0;
    float f = 1.0 / a;
    float u = f * glm::dot(s, h);
    if (u < 0.0 || u > 1.0)
        return false;
    glm::vec3 q = glm::cross(s, edge1);
    float v = f * glm::dot(ray.direction, q);
    if (v < 0.0 || u + v > 1.0)
        return false;
    // At this stage we can compute t to find out where the intersection point is on the line.
    float t = f * glm::dot(edge2, q);
    if (t > EPSILON && t < ray.t) // ray intersection
    {
        ray.t = t;
        hitInfo.normal = glm::normalize(glm::cross(edge1, edge2));
        hitInfo.barycentricCoord = glm::vec3(1 - u - v, u, v);
        return true;
    } else // This means that there is a line intersection but not a ray intersection.
        return false;
}

/// Input: a sphere with the following attributes: sphere.radius, sphere.center
/// Output: if intersects then modify the hit parameter ray.t and return true, otherwise return false
bool intersectRayWithShape(const Sphere& sphere, Ray& ray, HitInfo& hitInfo)
{
    float t = ray.t;
    ray.origin -= sphere.center;
    float a = std::pow(ray.direction.x, 2.0f) + std::pow(ray.direction.y, 2.0f) + std::pow(ray.direction.z, 2.0f);
    float b = 2 * (ray.direction.x * ray.origin.x + ray.direction.y * ray.origin.y + ray.direction.z * ray.origin.z);
    float c = std::pow(ray.origin.x, 2.0f) + std::pow(ray.origin.y, 2.0f) + std::pow(ray.origin.z, 2.0f)
        - std::pow(sphere.radius, 2.0f);

    ray.origin += sphere.center;
    float discriminant = b * b - 4 * a * c;

    if (discriminant < 0)
        return false;
    else if (discriminant == 0) {
        ray.t = std::min(-b / (2 * a), ray.t);
        if (ray.t == 0) {
            ray.t = t; // revert ray.t if no intersection
            return false;
        } 
        hitInfo.normal = glm::normalize(ray.origin + ray.direction * ray.t - sphere.center);
        hitInfo.material = sphere.material;
        return true;
    } else {
        float t1 = ((-b + std::sqrt(discriminant)) / (2 * a));
        float t2 = ((-b - std::sqrt(discriminant)) / (2 * a));
        float t;
        if (t2 < 0)
            t = t1;
        else
            t = std::min(t1, t2);
        ray.t = std::min(t, ray.t);
        if (ray.t == 0) {
            ray.t = t; // revert ray.t if no intersection
            return false;
        }
        hitInfo.normal = glm::normalize(ray.origin + ray.direction * ray.t - sphere.center);
        hitInfo.material = sphere.material;
        return true;
    }
}

/// Input: an axis-aligned bounding box with the following parameters: minimum coordinates box.lower and maximum
/// coordinates box.upper Output: if intersects then modify the hit parameter ray.t and return true, otherwise return
/// false
bool intersectRayWithShape(const AxisAlignedBox& box, Ray& ray)
{
    float txmin = (box.lower.x - ray.origin.x) / ray.direction.x;
    float txmax = (box.upper.x - ray.origin.x) / ray.direction.x;
    float tymin = (box.lower.y - ray.origin.y) / ray.direction.y;
    float tymax = (box.upper.y - ray.origin.y) / ray.direction.y;
    float tzmin = (box.lower.z - ray.origin.z) / ray.direction.z;
    float tzmax = (box.upper.z - ray.origin.z) / ray.direction.z;

    float tinx = std::min(txmin, txmax);
    float toutx = std::max(txmin, txmax);
    float tiny = std::min(tymin, tymax);
    float touty = std::max(tymin, tymax);
    float tinz = std::min(tzmin, tzmax);
    float toutz = std::max(tzmin, tzmax);

    float tin = std::max(std::max(tinx, tiny), tinz);
    float tout = std::min(std::min(toutx, touty), toutz);

    /*if (tin > tout || tout < 0)
        return false;

    if (tin < 0) 
        ray.t = std::min(tout, ray.t);
    else
        ray.t = std::min(tin, ray.t);

    if (ray.t == 0)
        return false;
    return true;*/
    if ((tin > tout) || (tout < 0))
        return -1;

    if (tin < 0)
        tin = 0;
    return tin;
}

float intersectRayWithShapeFloat(const AxisAlignedBox& box, Ray& ray)
{
    float txmin = (box.lower.x - ray.origin.x) / ray.direction.x;
    float txmax = (box.upper.x - ray.origin.x) / ray.direction.x;
    float tymin = (box.lower.y - ray.origin.y) / ray.direction.y;
    float tymax = (box.upper.y - ray.origin.y) / ray.direction.y;
    float tzmin = (box.lower.z - ray.origin.z) / ray.direction.z;
    float tzmax = (box.upper.z - ray.origin.z) / ray.direction.z;

    float tinx = std::min(txmin, txmax);
    float toutx = std::max(txmin, txmax);
    float tiny = std::min(tymin, tymax);
    float touty = std::max(tymin, tymax);
    float tinz = std::min(tzmin, tzmax);
    float toutz = std::max(tzmin, tzmax);

    float tin = std::max(std::max(tinx, tiny), tinz);
    float tout = std::min(std::min(toutx, touty), toutz);

    /*if (tin > tout || tout < 0)
        return false;

    if (tin < 0)
        ray.t = std::min(tout, ray.t);
    else
        ray.t = std::min(tin, ray.t);

    if (ray.t == 0)
        return false;
    return true;*/
    if ((tin > tout) || (tout < 0))
        return -1;

    if (tin < 0)
        tin = 0;
    return tin;
}

bool intersectShapeWithoutModifying(const AxisAlignedBox& box, Ray& ray)
{
    float initialT = ray.t;

    float txmin = (box.lower.x - ray.origin.x) / ray.direction.x;
    float txmax = (box.upper.x - ray.origin.x) / ray.direction.x;
    float tymin = (box.lower.y - ray.origin.y) / ray.direction.y;
    float tymax = (box.upper.y - ray.origin.y) / ray.direction.y;
    float tzmin = (box.lower.z - ray.origin.z) / ray.direction.z;
    float tzmax = (box.upper.z - ray.origin.z) / ray.direction.z;

    float tinx = std::min(txmin, txmax);
    float toutx = std::max(txmin, txmax);
    float tiny = std::min(tymin, tymax);
    float touty = std::max(tymin, tymax);
    float tinz = std::min(tzmin, tzmax);
    float toutz = std::max(tzmin, tzmax);

    float tin = std::max(std::max(tinx, tiny), tinz);
    float tout = std::min(std::min(toutx, touty), toutz);

    if (tin > tout || tout < 0)
        return false;

    if (tin < 0)
        ray.t = std::min(tout, ray.t);
    else
        ray.t = std::min(tin, ray.t);

    if (ray.t == 0) {
        ray.t = initialT;
        return false;
    }

    ray.t = initialT;
    return true;
}
