#include "render.h"
#include "intersect.h"
#include "light.h"
#include "screen.h"
#include <framework/trackball.h>
#ifdef NDEBUG
#include <omp.h>
#endif
#include <iostream>

glm::vec3 getFinalColor(const Scene& scene, const BvhInterface& bvh, Ray ray, const Features& features, int rayDepth)
{
    HitInfo hitInfo;
    if (bvh.intersect(ray, hitInfo, features)) {

        glm::vec3 Lo = computeLightContribution(scene, bvh, features, ray, hitInfo);

        if (features.enableRecursive) {
            Ray reflection = computeReflectionRay(ray, hitInfo);
            // TODO: put your own implementation of recursive ray tracing here.
            glm::vec3 reflectedColor = computeLightContribution(scene, bvh, features, reflection, hitInfo);
            reflectedColor += getFinalColor(scene, bvh, reflection, features, rayDepth + 1);
            Lo += hitInfo.material.ks * reflectedColor;
        }

        // Visual debug for shading + recursive ray tracing
        drawRay(ray, Lo); 

        drawNormal(ray, hitInfo);

        // Draw shadow rays
        if (features.enableHardShadow)
            drawShadowRays(ray, scene, bvh, features);

        // Set the color of the pixel if the ray hits.
        //return glm::abs(hitInfo.normal);
        //return glm::abs(hitInfo.barycentricCoords);
        return Lo;
    } else {
        // Draw a red debug ray if the ray missed.
        drawRay(ray, glm::vec3(1.0f, 0.0f, 0.0f));
        // Set the color of the pixel to black if the ray misses.
        return glm::vec3(0.0f);
    }
}

void renderRayTracing(
    const Scene& scene, const Trackball& camera, const BvhInterface& bvh, Screen& screen, const Features& features)
{
    glm::ivec2 windowResolution = screen.resolution();
    //int i = 0;
    // Enable multi threading in Release mode
#ifdef NDEBUG
#pragma omp parallel for schedule(guided)
#endif
    for (int y = 0; y < windowResolution.y; y++) {
        for (int x = 0; x != windowResolution.x; x++) {
            // NOTE: (-1, -1) at the bottom left of the screen, (+1, +1) at the top right of the screen.
            const glm::vec2 normalizedPixelPos { float(x) / float(windowResolution.x) * 2.0f - 1.0f,
                float(y) / float(windowResolution.y) * 2.0f - 1.0f };
            const Ray cameraRay = camera.generateRay(normalizedPixelPos);
            screen.setPixel(x, y, getFinalColor(scene, bvh, cameraRay, features));
            //i++;
            //std::cout << i * 100 / (windowResolution.y * windowResolution.x) << "% - Rendered " << i << " out of "
            //          << windowResolution.y * windowResolution.x
            //          << "pixels                                                             \r" << std::flush;
        }
    }
}