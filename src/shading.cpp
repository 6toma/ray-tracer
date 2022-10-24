#include "texture.h"
#include <cmath>
#include <glm/geometric.hpp>
#include <shading.h>

const glm::vec3 computeShading(const glm::vec3& lightPosition, const glm::vec3& lightColor, const Features& features, Ray ray, HitInfo hitInfo)
{
    if (!features.enableShading) // flag : "enableShading: when disabled the material diffuse color (kd) will be used during rendering."
        return hitInfo.material.kd;
   
    glm::vec3 pointOnPlane = ray.origin + ray.t * ray.direction;
    glm::vec3 pointToLight = glm::normalize(lightPosition - pointOnPlane);

    glm::vec3 diffuseLight = lightColor * hitInfo.material.kd * std::max(glm::dot(hitInfo.normal, pointToLight), 0.0f);

    // ray.origin is also the position of the camera in space, ray starts from our POV
    glm::vec3 viewVector = glm::normalize(ray.origin - pointOnPlane); 
    glm::vec3 reflectedVector = glm::reflect(-pointToLight, hitInfo.normal);

    glm::vec3 specularLight = lightColor * hitInfo.material.ks * pow(std::max(glm::dot(reflectedVector, viewVector), 0.0f), hitInfo.material.shininess);

    return diffuseLight + specularLight;
}


const Ray computeReflectionRay (Ray ray, HitInfo hitInfo)
{
    // Do NOT use glm::reflect!! write your own code.
    Ray reflectionRay {};

    if (hitInfo.material.ks != glm::vec3{ 0, 0, 0 }) { // if ks is black, then there is no specular reflected ray
        // origin of the reflection ray is the point on the surface
        reflectionRay.origin = ray.origin + ray.t * ray.direction;
        // ray.t and reflectionRay.t should be equal if we want the reflection ray to have the same magnitude as our ray
        reflectionRay.t = ray.t;
        // to get vector from origin to point on the surface we do: ray.origin - pointOnSurface, where the point is
        // ray.origin + ray.t * ray.direction
        glm::vec3 fromPointToOrigin = glm::normalize(
            -ray.t * ray.direction); // we can normalize since we only need the direction of the reflected vector
        reflectionRay.direction = 2 * glm::dot(hitInfo.normal, fromPointToOrigin) * hitInfo.normal - fromPointToOrigin;
    }

    return reflectionRay;
}