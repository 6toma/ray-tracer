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

glm::vec3 getFinalColor(const Scene& scene, const BvhInterface& bvh, Ray ray, const Features& features, int rayDepth)
{
    HitInfo hitInfo;
    if (bvh.intersect(ray, hitInfo, features)) {

        uint64_t rand_state[4] {
            ray.origin.x * 1000,
            ray.origin.y * 1000,
            ray.direction.x * 1000,
            rayDepth * 1000,
        };

        glm::vec3 Lo = computeLightContribution(scene, bvh, features, ray, hitInfo);

        if (features.enableRecursive && rayDepth < features.maxRayDepth) {
            if (hitInfo.material.transparency < 1 && features.extra.enableTransparency) {
                Ray trans {
                    ray.origin + ray.direction * ray.t * 1.000001f,
                    ray.direction,
                    std::numeric_limits<float>::max(),
                };
                glm::vec3 transColor(0);

                if (features.extra.enableTranslucency) {
                    glm::vec3 transX = (trans.direction == glm::vec3(1, 0, 0))
                        ? glm::normalize(glm::cross(trans.direction, glm::vec3(0, 0, 1)))
                        : glm::normalize(glm::cross(trans.direction, glm::vec3(1, 0, 0)));

                    glm::vec3 transY = glm::cross(trans.direction, transX);

                    int goodSamples = 0;
                    for (int i = 0; i < features.extra.translucentSamples; i++) {
                        float theta = to_double(next_rand()) * 2 * std::numbers::pi;
                        float r = std::sqrt(to_double(next_rand()));

                        if (enableDebugDraw) {
                            theta = to_double(next_rand(rand_state)) * 2 * std::numbers::pi;
                            r = std::sqrt(to_double(next_rand(rand_state)));
                        }

                        Ray translucent {
                            trans.origin,
                            trans.direction
                                + (transX * cos(theta) + transY * sin(theta)) * r / hitInfo.material.shininess,
                            std::numeric_limits<float>::max(),
                        };

                        translucent.origin += 0.000001f * translucent.direction;

                        if (std::signbit(dot(translucent.direction, hitInfo.normal))
                            == std::signbit(dot(trans.direction, hitInfo.normal))) {
                            transColor += getFinalColor(scene, bvh, translucent, features, rayDepth + 1);
                            goodSamples++;
                        }
                    }
                    transColor /= goodSamples;
                    Lo = Lo * hitInfo.material.transparency + transColor * (1 - hitInfo.material.transparency);
                } else
                    Lo = Lo * hitInfo.material.transparency
                        + getFinalColor(scene, bvh, trans, features, rayDepth + 1)
                            * (1 - hitInfo.material.transparency);
            }

            if (hitInfo.material.ks != glm::vec3(0)) {
                Ray reflection = computeReflectionRay(ray, hitInfo);
                glm::vec3 reflectedColor(0);

                if (features.extra.enableGlossyReflection) {
                    glm::vec3 reflectionX = (reflection.direction == glm::vec3(1, 0, 0))
                        ? glm::normalize(glm::cross(reflection.direction, glm::vec3(0, 0, 1)))
                        : glm::normalize(glm::cross(reflection.direction, glm::vec3(1, 0, 0)));

                    glm::vec3 reflectionY = glm::cross(reflection.direction, reflectionX);

                    int goodSamples = 0;
                    for (int i = 0; i < features.extra.glossySamples; i++) {
                        float theta = to_double(next_rand()) * 2 * std::numbers::pi;
                        float r = std::sqrt(to_double(next_rand()));

                        if (enableDebugDraw) {
                            theta = to_double(next_rand(rand_state)) * 2 * std::numbers::pi;
                            r = std::sqrt(to_double(next_rand(rand_state)));
                        }

                        Ray gloss {
                            reflection.origin,
                            reflection.direction
                                + (reflectionX * cos(theta) + reflectionY * sin(theta)) * r
                                    / hitInfo.material.shininess,
                            std::numeric_limits<float>::max(),
                        };

                        gloss.origin += 0.000001f * gloss.direction;

                        if (dot(gloss.direction, hitInfo.normal) > 0) {
                            reflectedColor += getFinalColor(scene, bvh, gloss, features, rayDepth + 1);
                            goodSamples++;
                        }
                    }
                    reflectedColor /= goodSamples;

                } else {
                    // reflectedColor = computeLightContribution(scene, bvh, features, reflection, hitInfo);
                    reflectedColor += getFinalColor(scene, bvh, reflection, features, rayDepth + 1);
                }

                Lo += hitInfo.material.ks * reflectedColor;
            }
        }

        // Visual debug for shading + recursive ray tracing
        drawRay(ray, Lo);

        if (features.debug.drawHitNormal)
            drawNormal(ray, hitInfo);

        // Draw shadow rays
        // if (features.enableHardShadow)
        //    drawShadowRays(ray, scene, bvh, features);

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
        glm::dot(camera.position() + camera.forward() * features.extra.focalLength, camera.forward()),
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
            const Ray cameraRay = camera.generateRay(normalizedPixelPos);
            // screen.setPixel(x, y, getFinalColor(scene, bvh, cameraRay, features));

            glm::vec3 pixelColor;
            if (features.extra.enableDepthOfField)
                pixelColor = DOFColor(scene, bvh, focalPlane, cameraRay, features, glm::mat2x3(apertureX, apertureY));
            else
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

// For later use, dw abt this
// Calculates the ratio between reflection and refraction
// float fresnel(const Ray& ray, const HitInfo& hitInfo, float IORfrom, float IORto)
//{
//    // Shlick's approximation for Fresnel
//    float r0 = ((IORfrom - IORto) * (IORfrom - IORto)) / ((IORfrom + IORto) * (IORfrom + IORto));
//    float cosTheta = -glm::dot(glm::normalize(ray.direction), hitInfo.normal);
//    if (IORfrom > IORto) {
//        float sinTheta2 = (1.0 - cosTheta * cosTheta) * IORfrom * IORfrom / (IORto * IORto);
//        if (sinTheta2 > 1.0)
//            // Total internal
//            return 1.0;
//        cosTheta = std::sqrt(1.0 - sinTheta2);
//    }
//}