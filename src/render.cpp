#include "render.h"
#include "intersect.h"
#include "light.h"
#include "rand_utils.h"
#include "screen.h"
#include "texture.h"
#include <framework/trackball.h>
#ifdef NDEBUG
#include <omp.h>
#endif
#include <iostream>
#include <numbers>

static const std::filesystem::path dataDirPath { DATA_DIR };
Image img(dataDirPath / "lake.png");
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
                        float theta = static_cast<float>(rand()) / static_cast<float>(RAND_MAX) * 2 * std::numbers::pi;
                        float r = std::sqrt(static_cast<float>(rand()) / static_cast<float>(RAND_MAX));

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
                        float theta = static_cast<float>(rand()) / static_cast<float>(RAND_MAX) * 2 * std::numbers::pi;
                        float r = std::sqrt(static_cast<float>(rand()) / static_cast<float>(RAND_MAX));

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
        // Set the color of the pixel if the ray misses.
        glm::vec3 defaultcolor = acquireTexelEnvironment(img, ray.direction, features);
        if (features.extra.enableEnvironmentMapping)
            drawRay(ray, defaultcolor);
        else
            drawRay(ray, glm::vec3(1.0f, 0.0f, 0.0f));
        return defaultcolor;
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
            Ray cameraRay = camera.generateRay(normalizedPixelPos);
            glm::vec3 pixelColor { 0.0f };
            if (features.extra.enableMotionBlur) {
                int sampleAmount = 200;
                for (int i = 0; i < sampleAmount; i++) {
                    float r = -static_cast<float>(rand()) / static_cast<float>(RAND_MAX) * features.extra.motionSpeed;
                    glm::vec3 offset = features.extra.motionDirection * r;
                    cameraRay.origin += offset;
                    pixelColor
                        += DOFColor(scene, bvh, focalPlane, cameraRay, features, glm::mat2x3(apertureX, apertureY));
                    cameraRay.origin -= offset;
                }
                pixelColor /= sampleAmount;
            } else
                pixelColor = DOFColor(scene, bvh, focalPlane, cameraRay, features, glm::mat2x3(apertureX, apertureY));
            screen.setPixel(x, y, pixelColor);
        }
    }

    if (features.extra.enableBloomEffect) {
        std::vector<glm::vec3> pixels = screen.pixels();
        for (int y = 0; y < windowResolution.y; y++) {
            for (int x = 0; x != windowResolution.x; x++) {
                glm::vec3 pixelColor = pixels[screen.indexAt(x, y)];
                if (pixelColor != glm::vec3(0)) {
                    glm::vec3 filteredPixelColor = glm::vec3(0);
                    float grayscaledValue = std::max(pixelColor.x, std::max(pixelColor.y, pixelColor.z));
                    if (grayscaledValue >= features.extra.bloomThreshold) {
                        glm::vec3 sum(0);
                        //2 pass gaussian blur
                        if (!(x < 4 || y < 4 || x + 4 >= windowResolution.x || y + 4 >= windowResolution.y)) {
                            //pixel colors are multiplied by weights from a gaussian distribution
                            //pass on the horizontal axis
                            sum += pixels[screen.indexAt(x - 4, y)] * 0.016216f
                                + pixels[screen.indexAt(x - 3, y)] * 0.054054f
                                + pixels[screen.indexAt(x - 2, y)] * 0.1216216f
                                + pixels[screen.indexAt(x - 1, y)] * 0.1945946f
                                + pixels[screen.indexAt(x, y)] * 0.227027f
                                + pixels[screen.indexAt(x + 1, y)] * 0.1945946f
                                + pixels[screen.indexAt(x + 2, y)] * 0.1216216f
                                + pixels[screen.indexAt(x + 3, y)] * 0.054054f
                                + pixels[screen.indexAt(x + 4, y)] * 0.016216f;

                            //pass on the vertical axis
                            sum += pixels[screen.indexAt(x, y - 4)] * 0.016216f
                                + pixels[screen.indexAt(x, y - 3)] * 0.054054f
                                + pixels[screen.indexAt(x, y - 2)] * 0.1216216f
                                + pixels[screen.indexAt(x, y - 1)] * 0.1945946f
                                + pixels[screen.indexAt(x, y)] * 0.227027f
                                + pixels[screen.indexAt(x, y + 1)] * 0.1945946f
                                + pixels[screen.indexAt(x, y + 2)] * 0.1216216f
                                + pixels[screen.indexAt(x, y + 3)] * 0.054054f
                                + pixels[screen.indexAt(x, y + 4)] * 0.016216f;

                            sum /= 2;
                            filteredPixelColor += sum * features.extra.bloomIntensity;
                        }
                    }
                    if (filteredPixelColor != glm::vec3(0)) {
                        pixelColor += filteredPixelColor;
                        if (!(x < 1 || y < 1 || x + 1 >= windowResolution.x || y + 1 >= windowResolution.y)) {
                            screen.setPixel(x - 1, y - 1, pixelColor);
                            screen.setPixel(x - 1, y, pixelColor);
                            screen.setPixel(x - 1, y + 1, pixelColor);
                            screen.setPixel(x, y - 1, pixelColor);
                            screen.setPixel(x, y, pixelColor);
                            screen.setPixel(x, y + 1, pixelColor);
                            screen.setPixel(x + 1, y - 1, pixelColor);
                            screen.setPixel(x + 1, y, pixelColor);
                            screen.setPixel(x + 1, y + 1, pixelColor);
                        }
                    } else
                        screen.setPixel(x, y, pixelColor);
                }
            }
            
        }
    }
}

glm::vec3 DOFColor(
    const Scene& scene, const BvhInterface& bvh, const Plane& focalPlane, const Ray& cameraRay,
    const Features& features, const glm::mat2x3& apertureBasis
)
{
    if (!features.extra.enableDepthOfField)
        return getFinalColor(scene, bvh, cameraRay, features);

    float t = (focalPlane.D - glm::dot(focalPlane.normal, cameraRay.origin))
        / glm::dot(focalPlane.normal, cameraRay.direction);

    glm::vec3 pixelColor(0);
    for (int i = 0; i < features.extra.DOFSamples; i++) {
        float theta = static_cast<float>(rand()) / static_cast<float>(RAND_MAX) * 2 * std::numbers::pi;
        float r = std::sqrt(static_cast<float>(rand()) / static_cast<float>(RAND_MAX));
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