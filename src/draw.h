#pragma once
#include "scene.h"
#include <framework/mesh.h>
#include <framework/ray.h>
#include <utility> // std::forward
#include <bvh_interface.h>
#include <glm/mat2x3.hpp>

// Flag to enable/disable the debug drawing.
extern bool enableDebugDraw;

// Add your own custom visual debug draw functions here then implement it in draw.cpp.
// You are free to modify the example one however you like.
//
// Please add following lines on top inside your custom draw function:
//
// if (!enableDebugDraw)
//     return;
//
void drawExampleOfCustomVisualDebug();

void drawLine(const glm::vec3 start, const glm::vec3 end, const glm::vec3& color);
void drawLine(const Ray& ray, const glm::vec3& color);
void drawRay(const Ray& ray, const glm::vec3& color = glm::vec3(1.0f));
void drawShadowRays(const Ray& ray, const Scene& scene, const BvhInterface& bvh, const Features& features);
void drawNormal(const Ray& ray, const HitInfo& hitInfo);
void drawNormals(const Scene& scene, int interpolationLevel);
void drawDOF(
    const Scene& scene, const BvhInterface& bvh, const Plane& focalPlane, const Ray& cameraRay,
    const Features& features, const glm::mat2x3& apertureBasis
);

void drawAABB(const AxisAlignedBox& box, DrawMode drawMode = DrawMode::Filled, const glm::vec3& color = glm::vec3(1.0f), float transparency = 1.0f);

void drawTriangle (const Vertex& v0, const Vertex& v1, const Vertex& v2);
void drawMesh(const Mesh& mesh);
void drawSphere(const Sphere& sphere);
void drawSphere(const glm::vec3& center, float radius, const glm::vec3& color = glm::vec3(1.0f));
void drawScene(const Scene& scene);

