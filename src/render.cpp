#include "render.h"
#include "intersect.h"
#include "light.h"
#include "rand_utils.h"
#include "screen.h"
#include <framework/trackball.h>
#ifdef NDEBUG
#include <omp.h>
#endif
#include <iostream>
#include <numbers>
MotionBlurSetting motionBlurSetting {};
glm::vec3 getFinalColor(const Scene& scene, const BvhInterface& bvh, Ray ray, const Features& features, int rayDepth)
{
    HitInfo hitInfo;
    if (bvh.intersect(ray, hitInfo, features)) {

        glm::vec3 Lo = computeLightContribution(scene, bvh, features, ray, hitInfo);

        if (features.enableRecursive && rayDepth < features.maxRayDepth && hitInfo.material.ks == glm::vec3(0)) {
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
        // return glm::abs(hitInfo.normal);
        // return glm::abs(hitInfo.barycentricCoords);
        return Lo;
    } else {
        // Draw a red debug ray if the ray missed.
        drawRay(ray, glm::vec3(1.0f, 0.0f, 0.0f));
        // Set the color of the pixel to black if the ray misses.
        return glm::vec3(0.0f);
    }
}

void renderRayTracing(
    const Scene& scene, const Trackball& camera, const BvhInterface& bvh, Screen& screen, const Features& features
)
{
    glm::ivec2 windowResolution = screen.resolution();

    Plane focalPlane {
        glm::dot(camera.position() + camera.forward() * glm::vec3(features.extra.focalLength), camera.forward()),
        camera.forward(),
    };

    glm::vec3 apertureX = (camera.forward() == glm::vec3(1, 0, 0))
        ? glm::normalize(glm::cross(camera.forward(), glm::vec3(0, 0, 1)))
        : glm::normalize(glm::cross(camera.forward(), glm::vec3(1, 0, 0)));

    glm::vec3 apertureY = glm::cross(camera.forward(), apertureX);

    // Enable multi threading in Release mode
#ifdef NDEBUG
#pragma omp parallel for schedule(guided)
#endif
    for (int y = 0; y < windowResolution.y; y++) {
        for (int x = 0; x != windowResolution.x; x++) {
            // NOTE: (-1, -1) at the bottom left of the screen, (+1, +1) at the top right of the screen.
            const glm::vec2 normalizedPixelPos {
                float(x) / float(windowResolution.x) * 2.0f - 1.0f,
                float(y) / float(windowResolution.y) * 2.0f - 1.0f,
            };
            Ray cameraRay = camera.generateRay(normalizedPixelPos);
            glm::vec3 pixelColor {0.0f};
            if (features.extra.enableDepthOfField)
                pixelColor = DOFColor(scene, bvh, focalPlane, cameraRay, features, glm::mat2x3(apertureX, apertureY));
            else if (features.extra.enableMotionBlur) {
                int sampleAmount = 200;
                float cof = 1.0 / (float)(sampleAmount);
                for (int i = 0; i < sampleAmount; i++) {
                    float r = -static_cast<float>(rand()) / static_cast<float>(RAND_MAX) * motionBlurSetting.speed;
                    glm::vec3 offset = motionBlurSetting.movingDirection * r;
                    cameraRay.origin += offset;
                    pixelColor += getFinalColor(scene, bvh, cameraRay, features) * cof;
                    cameraRay.origin -= offset;
                }
            } else
                pixelColor = getFinalColor(scene, bvh, cameraRay, features);
            screen.setPixel(x, y, pixelColor);  
        }
    }
}

glm::vec3 DOFColor(
    const Scene& scene, const BvhInterface& bvh, const Plane& focalPlane, const Ray& cameraRay,
    const Features& features, const glm::mat2x3& apertureBasis
)
{
    float t = (focalPlane.D - glm::dot(focalPlane.normal, cameraRay.origin))
        / glm::dot(focalPlane.normal, cameraRay.direction);

    glm::vec3 pixelColor(0);
    for (int i = 0; i < features.extra.DOFSamples; i++) {
        float theta = to_double(next_rand()) * 2 * std::numbers::pi;
        float r = std::sqrt(to_double(next_rand()));
        glm::vec2 offset = glm::vec2(cos(theta), sin(theta)) * r * features.extra.apertureRadius;

        const Ray DOFRay {
            cameraRay.origin + apertureBasis * offset,
            glm::normalize((cameraRay.direction * t) - apertureBasis * offset),
            std::numeric_limits<float>::max(),
        };

        pixelColor += getFinalColor(scene, bvh, DOFRay, features);
    }

    return pixelColor / glm::vec3(features.extra.DOFSamples);
}