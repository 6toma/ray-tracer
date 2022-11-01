#pragma once
#include <framework/disable_all_warnings.h>
DISABLE_WARNINGS_PUSH()
#include <glm/gtc/type_ptr.hpp>
#include <glm/vec3.hpp>
DISABLE_WARNINGS_POP()
#include <framework/ray.h>
#include "config.h"
// Forward declarations.
struct Scene;
class Screen;
class Trackball;
class BvhInterface;
struct Features;
extern MotionBlurSetting motionBlurSetting;
// Main rendering function.
void renderRayTracing(const Scene& scene, const Trackball& camera, const BvhInterface& bvh, Screen& screen, const Features& features);
    // Get the color of a ray.
glm::vec3 getFinalColor(const Scene& scene, const BvhInterface& bvh, Ray ray, const Features& features, int rayDepth = 0);

glm::vec3 DOFColor(
    const Scene& scene, const BvhInterface& bvh, const Plane& focalPlane, const Ray& cameraRay,
    const Features& features, const glm::mat2x3& apertureBasis
);