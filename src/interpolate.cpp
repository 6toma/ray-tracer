#include "interpolate.h"
#include <glm/geometric.hpp>

glm::vec3 computeBarycentricCoord(const glm::vec3& v0, const glm::vec3& v1, const glm::vec3& v2, const glm::vec3& p)
{
    float area = glm::length(glm::cross(v1 - v0, v2 - v0)) / 2;
    glm::vec3 coords = glm::vec3(
        0, glm::length(glm::cross(v0 - p, v2 - p)) / (2 * area), glm::length(glm::cross(v0 - p, v1 - p)) / (2 * area));
    coords.x = 1 - coords.y - coords.z;

    return coords;
}

glm::vec3 interpolateNormal(
    const glm::vec3& n0, const glm::vec3& n1, const glm::vec3& n2, const glm::vec3 barycentricCoord)
{
    return barycentricCoord.x * n0 + barycentricCoord.y * n1 + barycentricCoord.z * n2;
}

glm::vec2 interpolateTexCoord(
    const glm::vec2& t0, const glm::vec2& t1, const glm::vec2& t2, const glm::vec3 barycentricCoord)
{
    return barycentricCoord.x * t0 + barycentricCoord.y * t1 + barycentricCoord.z * t2;
}
