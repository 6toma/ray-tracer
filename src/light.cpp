#include "light.h"
#include "config.h"
// Suppress warnings in third-party code.
#include <framework/disable_all_warnings.h>
DISABLE_WARNINGS_PUSH()
#include <glm/geometric.hpp>
DISABLE_WARNINGS_POP()
#include <cmath>
#include <random>

std::random_device rd;
std::minstd_rand gen(rd());
std::uniform_real_distribution<> dis(0, 1);

static inline uint64_t rotl(const uint64_t x, int k) { return (x << k) | (x >> (64 - k)); }
 
static inline double to_double(uint64_t x)
{
    const union {
        uint64_t i;
        double d;
    } u = { .i = UINT64_C(0x3FF) << 52 | x >> 12 };
    return u.d - 1.0;
}

// seed, randomly generated with a d10
uint64_t shuffle_table[4] {
  4550963226,
  4884,
  2,
  724
};
uint64_t next_rand(void)
{
    const uint64_t result = rotl(shuffle_table[1] * 5, 7) * 9;

    const uint64_t t = shuffle_table[1] << 17;

    shuffle_table[2] ^= shuffle_table[0];
    shuffle_table[3] ^= shuffle_table[1];
    shuffle_table[1] ^= shuffle_table[2];
    shuffle_table[0] ^= shuffle_table[3];

    shuffle_table[2] ^= t;

    shuffle_table[3] = rotl(shuffle_table[3], 45);

    return result;
}

// samples a segment light source
// you should fill in the vectors position and color with the sampled position and color
void sampleSegmentLight(const SegmentLight& segmentLight, glm::vec3& position, glm::vec3& color)
{
    float t = to_double(next_rand());
    //float t = dis(gen);
    position = segmentLight.endpoint0 * t + segmentLight.endpoint1 * (1 - t);
    color = segmentLight.color0 * t + segmentLight.color1 * (1 - t);
}

// samples a parallelogram light source
// you should fill in the vectors position and color with the sampled position and color
void sampleParallelogramLight(const ParallelogramLight& parallelogramLight, glm::vec3& position, glm::vec3& color)
{
    float x = to_double(next_rand());
    float y = to_double(next_rand());
    //float x = dis(gen);
    //float y = dis(gen);
    position = parallelogramLight.v0 + parallelogramLight.edge01 * x + parallelogramLight.edge02 * y;
    color = parallelogramLight.color0 * x * y + parallelogramLight.color1 * (1 - x) * y
        + parallelogramLight.color2 * x * (1 - y) + parallelogramLight.color3 * (1 - x) * (1 - y);
}

// test the visibility at a given light sample
// returns 1.0 if sample is visible, 0.0 otherwise
float testVisibilityLightSample(const glm::vec3& samplePos, const glm::vec3& debugColor, const BvhInterface& bvh,
    const Features& features, Ray ray, HitInfo hitInfo)
{
    glm::vec3 pos = ray.origin + ray.direction * ray.t;
    glm::vec3 offset = glm::normalize(samplePos - pos) * glm::vec3(0.000001);
    Ray lightRay { pos + offset, samplePos - pos - offset, 1 };
    if (features.enableHardShadow && bvh.intersect(lightRay, hitInfo, features) && lightRay.t < 1)
        return 0.0;

    return 1.0;
}

// given an intersection, computes the contribution from all light sources at the intersection point
// in this method you should cycle the light sources and for each one compute their contribution
// don't forget to check for visibility (shadows!)

// Lights are stored in a single array (scene.lights) where each item can be either a PointLight, SegmentLight or
// ParallelogramLight. You can check whether a light at index i is a PointLight using std::holds_alternative:
// std::holds_alternative<PointLight>(scene.lights[i])
//
// If it is indeed a point light, you can "convert" it to the correct type using std::get:
// PointLight pointLight = std::get<PointLight>(scene.lights[i]);
//
//
// The code to iterate over the lights thus looks like this:
// for (const auto& light : scene.lights) {
//     if (std::holds_alternative<PointLight>(light)) {
//         const PointLight pointLight = std::get<PointLight>(light);
//         // Perform your calculations for a point light.
//     } else if (std::holds_alternative<SegmentLight>(light)) {
//         const SegmentLight segmentLight = std::get<SegmentLight>(light);
//         // Perform your calculations for a segment light.
//     } else if (std::holds_alternative<ParallelogramLight>(light)) {
//         const ParallelogramLight parallelogramLight = std::get<ParallelogramLight>(light);
//         // Perform your calculations for a parallelogram light.
//     }
// }
//
// Regarding the soft shadows for **other** light sources **extra** feature:
// To add a new light source, define your new light struct in scene.h and modify the Scene struct (also in scene.h)
// by adding your new custom light type to the lights std::variant. For example:
// std::vector<std::variant<PointLight, SegmentLight, ParallelogramLight, MyCustomLightType>> lights;
//
// You can add the light sources programmatically by creating a custom scene (modify the Custom case in the
// loadScene function in scene.cpp). Custom lights will not be visible in rasterization view.
glm::vec3 computeLightContribution(
    const Scene& scene, const BvhInterface& bvh, const Features& features, Ray ray, HitInfo hitInfo)
{
    /*if (!bvh.intersect(ray, hitInfo, features))
        return glm::vec3 { 0, 0, 0 };*/

    if (features.enableShading) {
        // If shading is enabled, compute the contribution from all lights.
        // TODO: replace this by your own implementation of shading
        glm::vec3 shading = { 0, 0, 0 };
        float totalSamples = 0;

        for (const auto& light : scene.lights) {
            if (std::holds_alternative<PointLight>(light)) {
                const PointLight pointLight = std::get<PointLight>(light);

                totalSamples++;

                shading += computeShading(pointLight.position, pointLight.color, features, ray, hitInfo)
                    * testVisibilityLightSample(pointLight.position, pointLight.color, bvh, features, ray, hitInfo);
            } else if (features.enableSoftShadow && std::holds_alternative<SegmentLight>(light)) {
                const SegmentLight segmentLight = std::get<SegmentLight>(light);

                glm::vec3 lightPosition, lightColor;
                int numSamples = glm::length(segmentLight.endpoint0 - segmentLight.endpoint1) * 100000;
                //int numSamples = 100000;
                totalSamples += numSamples;
                for (int i = 0; i < numSamples; i++) {
                    sampleSegmentLight(segmentLight, lightPosition, lightColor);

                    shading += computeShading(lightPosition, lightColor, features, ray, hitInfo)
                        * testVisibilityLightSample(lightPosition, lightColor, bvh, features, ray, hitInfo);
                }
            } else if (features.enableSoftShadow && std::holds_alternative<ParallelogramLight>(light)) {
                const ParallelogramLight parallelogramLight = std::get<ParallelogramLight>(light);

                glm::vec3 lightPosition, lightColor;
                int numSamples = glm::length(glm::cross(parallelogramLight.edge01, parallelogramLight.edge02)) * 1000;
                //int numSamples = 100000;
                totalSamples += numSamples;
                for (int i = 0; i < numSamples; i++) {
                    sampleParallelogramLight(parallelogramLight, lightPosition, lightColor);

                    shading += computeShading(lightPosition, lightColor, features, ray, hitInfo)
                        * testVisibilityLightSample(lightPosition, lightColor, bvh, features, ray, hitInfo);
                }
            }
        }

        if (totalSamples == 0)
            return glm::vec3(0);

        return shading / totalSamples;

    } else {
        // If shading is disabled, return the albedo of the material.
        return hitInfo.material.kd;
    }
}
