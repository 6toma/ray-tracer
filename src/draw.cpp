#include "draw.h"
#include "interpolate.h"
#include <framework/opengl_includes.h>
#include <framework/trackball.h>
#include "screen.h"
// Suppress warnings in third-party code.
#include <framework/disable_all_warnings.h>
DISABLE_WARNINGS_PUSH()
#ifdef __APPLE__
#include <OpenGL/GLU.h>
#else
#ifdef WIN32
// Windows.h includes a ton of stuff we don't need, this macro tells it to include less junk.
#define WIN32_LEAN_AND_MEAN
// Disable legacy macro of min/max which breaks completely valid C++ code (std::min/std::max won't work).
#define NOMINMAX
// GLU requires Windows.h on Windows :-(.
#include <Windows.h>
#endif
#include <GL/glu.h>
#endif
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/mat4x4.hpp>
DISABLE_WARNINGS_POP()
#include <algorithm>
#include <bvh_interface.h>
#include <light.h>
#include <numbers>
#include <rand_utils.h>
#include <render.h>

bool enableDebugDraw = false;

static void setMaterial(const Material& material)
{
    // Set the material color of the shape.
    const glm::vec4 kd4 { material.kd, 1.0f };
    glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, glm::value_ptr(kd4));

    const glm::vec4 zero { 0.0f };
    glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, glm::value_ptr(zero));
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, glm::value_ptr(zero));
}

void drawExampleOfCustomVisualDebug()
{
    glBegin(GL_TRIANGLES);
    glVertex3f(0.0f, 0.0f, 0.0f);
    glVertex3f(0.0f, 0.0f, 1.0f);
    glVertex3f(0.0f, 1.0f, 0.0f);
    glEnd();
}

void drawTriangle(const Vertex& v0, const Vertex& v1, const Vertex& v2)
{
    glBegin(GL_TRIANGLES);
    glNormal3fv(glm::value_ptr(v0.normal));
    glVertex3fv(glm::value_ptr(v0.position));
    glNormal3fv(glm::value_ptr(v1.normal));
    glVertex3fv(glm::value_ptr(v1.position));
    glNormal3fv(glm::value_ptr(v2.normal));
    glVertex3fv(glm::value_ptr(v2.position));
    glEnd();
}

void drawMesh(const Mesh& mesh)
{
    setMaterial(mesh.material);

    glBegin(GL_TRIANGLES);
    for (const auto& triangleIndex : mesh.triangles) {
        for (int i = 0; i < 3; i++) {
            const auto& vertex = mesh.vertices[triangleIndex[i]];
            glNormal3fv(glm::value_ptr(vertex.normal));
            glVertex3fv(glm::value_ptr(vertex.position));
        }
    }
    glEnd();
}

static void drawSphereInternal(const glm::vec3& center, float radius)
{
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    const glm::mat4 transform = glm::translate(glm::identity<glm::mat4>(), center);
    glMultMatrixf(glm::value_ptr(transform));
    auto quadric = gluNewQuadric();
    gluSphere(quadric, radius, 40, 20);
    gluDeleteQuadric(quadric);
    glPopMatrix();
}

void drawSphere(const Sphere& sphere)
{
    glPushAttrib(GL_ALL_ATTRIB_BITS);
    setMaterial(sphere.material);
    drawSphereInternal(sphere.center, sphere.radius);
    glPopAttrib();
}

void drawSphere(const glm::vec3& center, float radius, const glm::vec3& color /*= glm::vec3(1.0f)*/)
{
    glPushAttrib(GL_ALL_ATTRIB_BITS);
    glColor4f(color.r, color.g, color.b, 1.0f);
    glPolygonMode(GL_FRONT, GL_FILL);
    glPolygonMode(GL_BACK, GL_FILL);
    drawSphereInternal(center, radius);
    glPopAttrib();
}

static void drawAABBInternal(const AxisAlignedBox& box)
{
    glPushMatrix();

    // front      back
    // 3 ----- 2  7 ----- 6
    // |       |  |       |
    // |       |  |       |
    // 0 ------1  4 ------5

    glBegin(GL_QUADS);
    glNormal3f(0, 0, -1);
    glVertex3f(box.lower.x, box.upper.y, box.lower.z); // 3
    glVertex3f(box.upper.x, box.upper.y, box.lower.z); // 2
    glVertex3f(box.upper.x, box.lower.y, box.lower.z); // 1
    glVertex3f(box.lower.x, box.lower.y, box.lower.z); // 0

    glNormal3f(0, 0, 1);
    glVertex3f(box.upper.x, box.lower.y, box.upper.z); // 5
    glVertex3f(box.upper.x, box.upper.y, box.upper.z); // 6
    glVertex3f(box.lower.x, box.upper.y, box.upper.z); // 7
    glVertex3f(box.lower.x, box.lower.y, box.upper.z); // 4

    glNormal3f(1, 0, 0);
    glVertex3f(box.upper.x, box.upper.y, box.lower.z); // 2
    glVertex3f(box.upper.x, box.upper.y, box.upper.z); // 6
    glVertex3f(box.upper.x, box.lower.y, box.upper.z); // 5
    glVertex3f(box.upper.x, box.lower.y, box.lower.z); // 1

    glNormal3f(-1, 0, 0);
    glVertex3f(box.lower.x, box.lower.y, box.upper.z); // 4
    glVertex3f(box.lower.x, box.upper.y, box.upper.z); // 7
    glVertex3f(box.lower.x, box.upper.y, box.lower.z); // 3
    glVertex3f(box.lower.x, box.lower.y, box.lower.z); // 0

    glNormal3f(0, 1, 0);
    glVertex3f(box.lower.x, box.upper.y, box.upper.z); // 7
    glVertex3f(box.upper.x, box.upper.y, box.upper.z); // 6
    glVertex3f(box.upper.x, box.upper.y, box.lower.z); // 2
    glVertex3f(box.lower.x, box.upper.y, box.lower.z); // 3

    glNormal3f(0, -1, 0);
    glVertex3f(box.upper.x, box.lower.y, box.lower.z); // 1
    glVertex3f(box.upper.x, box.lower.y, box.upper.z); // 5
    glVertex3f(box.lower.x, box.lower.y, box.upper.z); // 4
    glVertex3f(box.lower.x, box.lower.y, box.lower.z); // 0
    glEnd();

    glPopMatrix();
}

void drawAABB(const AxisAlignedBox& box, DrawMode drawMode, const glm::vec3& color, float transparency)
{
    glPushAttrib(GL_ALL_ATTRIB_BITS);
    glColor4f(color.r, color.g, color.b, transparency);
    if (drawMode == DrawMode::Filled) {
        glPolygonMode(GL_FRONT, GL_FILL);
        glPolygonMode(GL_BACK, GL_FILL);
    } else {
        glPolygonMode(GL_FRONT, GL_LINE);
        glPolygonMode(GL_BACK, GL_LINE);
    }
    drawAABBInternal(box);
    glPopAttrib();
}

void drawScene(const Scene& scene)
{
    for (const auto& mesh : scene.meshes)
        drawMesh(mesh);
    for (const auto& sphere : scene.spheres)
        drawSphere(sphere);
}

void drawLine(const glm::vec3 start, const glm::vec3 end, const glm::vec3& color)
{
    if (!enableDebugDraw)
        return;

    glPushAttrib(GL_ALL_ATTRIB_BITS);
    glDisable(GL_LIGHTING);
    glBegin(GL_LINES);

    glColor3fv(glm::value_ptr(color));
    glVertex3fv(glm::value_ptr(start));
    glColor3fv(glm::value_ptr(color));
    glVertex3fv(glm::value_ptr(end));
    glEnd();
    glPopAttrib();
}

void drawLine(const Ray& ray, const glm::vec3& color)
{
    if (!enableDebugDraw)
        return;

    const glm::vec3 hitPoint = ray.origin + std::clamp(ray.t, 0.0f, 100.0f) * ray.direction;

    glPushAttrib(GL_ALL_ATTRIB_BITS);
    glDisable(GL_LIGHTING);
    glBegin(GL_LINES);

    glColor3fv(glm::value_ptr(color));
    glVertex3fv(glm::value_ptr(ray.origin));
    glColor3fv(glm::value_ptr(color));
    glVertex3fv(glm::value_ptr(hitPoint));

    glEnd();
    glPopAttrib();
}

void drawRay(const Ray& ray, const glm::vec3& color)
{
    if (!enableDebugDraw)
        return;

    const glm::vec3 hitPoint = ray.origin + std::clamp(ray.t, 0.0f, 100.0f) * ray.direction;
    const bool hit = (ray.t < std::numeric_limits<float>::max());

    glPushAttrib(GL_ALL_ATTRIB_BITS);
    glDisable(GL_LIGHTING);
    glBegin(GL_LINES);

    glColor3fv(glm::value_ptr(color));
    glVertex3fv(glm::value_ptr(ray.origin));
    glColor3fv(glm::value_ptr(color));
    glVertex3fv(glm::value_ptr(hitPoint));

    glEnd();

    if (hit)
        drawSphere(hitPoint, 0.005f, color);

    glPopAttrib();
}

void drawShadowRays(const Ray& ray, const Scene& scene, const BvhInterface& bvh, const Features& features)
{
    if (!enableDebugDraw)
        return;

    HitInfo hitInfo;
    uint64_t rand_state[4] { ray.origin.x * 1000, ray.origin.y * 1000, ray.direction.x * 1000, ray.direction.z * 1000 };

    for (const auto& light : scene.lights) {
        if (std::holds_alternative<PointLight>(light)) {
            const PointLight pointLight = std::get<PointLight>(light);
            glm::vec3 pos = ray.origin + ray.direction * ray.t;
            glm::vec3 offset = glm::normalize(pointLight.position - pos) * 0.000001f;
            Ray lightRay { pos + offset, pointLight.position - pos - offset, 1 };
            if (bvh.intersect(lightRay, hitInfo, features) && lightRay.t < 1) {
                drawRay(lightRay, glm::vec3(1, 0, 0));
            } else {
                lightRay.t = 1;
                drawRay(lightRay, glm::vec3(0, 1, 0));
            }

        } else if (features.enableSoftShadow && std::holds_alternative<SegmentLight>(light)) {
            const SegmentLight segmentLight = std::get<SegmentLight>(light);
            glm::vec3 pos = ray.origin + ray.direction * ray.t;

            glm::vec3 lightPosition, lightColor;
            int numSamples = glm::length(segmentLight.endpoint0 - segmentLight.endpoint1) * 100000;
            for (int i = 0; i < numSamples; i++) {
                float t = to_double(next_rand(rand_state));
                lightPosition = segmentLight.endpoint0 * t + segmentLight.endpoint1 * (1 - t);
                lightColor = segmentLight.color0 * t + segmentLight.color1 * (1 - t);

                glm::vec3 offset = glm::normalize(lightPosition - pos) * 0.000001f;
                Ray lightRay { pos + offset, lightPosition - pos - offset, 1 };
                if (bvh.intersect(lightRay, hitInfo, features) && lightRay.t < 1) {
                    drawRay(lightRay, glm::vec3(1, 0, 0));
                } else {
                    lightRay.t = 1;
                    drawRay(lightRay, glm::vec3(0, 1, 0));
                }
            }
        } else if (features.enableSoftShadow && std::holds_alternative<ParallelogramLight>(light)) {
            const ParallelogramLight parallelogramLight = std::get<ParallelogramLight>(light);
            glm::vec3 pos = ray.origin + ray.direction * ray.t;

            glm::vec3 lightPosition, lightColor;
            int numSamples = glm::length(glm::cross(parallelogramLight.edge01, parallelogramLight.edge02)) * 1000;
            for (int i = 0; i < numSamples; i++) {
                float x = to_double(next_rand(rand_state));
                float y = to_double(next_rand(rand_state));
                // float x = dis(gen);
                // float y = dis(gen);
                lightPosition = parallelogramLight.v0 + parallelogramLight.edge01 * x + parallelogramLight.edge02 * y;
                lightColor = parallelogramLight.color0 * x * y + parallelogramLight.color1 * (1 - x) * y
                    + parallelogramLight.color2 * x * (1 - y) + parallelogramLight.color3 * (1 - x) * (1 - y);

                glm::vec3 offset = glm::normalize(lightPosition - pos) * 0.000001f;
                Ray lightRay { pos + offset, lightPosition - pos - offset, 1 };
                if (bvh.intersect(lightRay, hitInfo, features) && lightRay.t < 1) {
                    drawRay(lightRay, glm::vec3(1, 0, 0));
                } else {
                    lightRay.t = 1;
                    drawRay(lightRay, glm::vec3(0, 1, 0));
                }
            }
        }
    }
}

void drawNormal(const Ray& ray, const HitInfo& hitInfo)
{
    if (!enableDebugDraw)
        return;

    drawLine(Ray { ray.origin + ray.direction * ray.t, hitInfo.normal, 0.1 }, glm::abs(hitInfo.normal));
}

void drawNormals(const Scene& scene, int interpolationLevel)
{
    if (!enableDebugDraw)
        return;

    for (const auto& mesh : scene.meshes) {
        for (const auto& triangleIndex : mesh.triangles) {
            glm::vec3 normals[3] {};
            for (int i = 0; i < 3; i++) {
                const auto& vertex = mesh.vertices[triangleIndex[i]];
                drawLine(Ray { vertex.position, vertex.normal, 0.1 }, glm::abs(vertex.normal));

                normals[i] = vertex.normal;
            }

            // this is dumb but i dont have the mental capacity to do it better now
            // lets say its optimization because loop unrolling lol
            if (interpolationLevel == 1) {
                glm::vec3 interNormal = interpolateNormal(normals[0], normals[1], normals[2], glm::vec3(1.0 / 3.0));
                glm::vec3 interPos
                    = (mesh.vertices[triangleIndex[0]].position + mesh.vertices[triangleIndex[1]].position
                       + mesh.vertices[triangleIndex[2]].position)
                    / 3.0f;

                drawLine(Ray { interPos, interNormal, 0.1 }, glm::abs(interNormal));
            } else if (interpolationLevel == 2) {
                glm::vec3 interNormal = interpolateNormal(normals[0], normals[1], normals[2], glm::vec3(0.6, 0.2, 0.2));
                glm::vec3 interPos = (mesh.vertices[triangleIndex[0]].position * 0.6f)
                    + (mesh.vertices[triangleIndex[1]].position + mesh.vertices[triangleIndex[2]].position) * 0.2f;
                drawLine(Ray { interPos, interNormal, 0.1 }, glm::abs(interNormal));

                interNormal = interpolateNormal(normals[0], normals[1], normals[2], glm::vec3(0.2, 0.6, 0.2));
                interPos = (mesh.vertices[triangleIndex[1]].position * 0.6f)
                    + (mesh.vertices[triangleIndex[0]].position + mesh.vertices[triangleIndex[2]].position) * 0.2f;
                drawLine(Ray { interPos, interNormal, 0.1 }, glm::abs(interNormal));

                interNormal = interpolateNormal(normals[0], normals[1], normals[2], glm::vec3(0.2, 0.2, 0.6));
                interPos = (mesh.vertices[triangleIndex[2]].position * 0.6f)
                    + (mesh.vertices[triangleIndex[1]].position + mesh.vertices[triangleIndex[0]].position) * 0.2f;
                drawLine(Ray { interPos, interNormal, 0.1 }, glm::abs(interNormal));

                interNormal = interpolateNormal(normals[0], normals[1], normals[2], glm::vec3(0.4, 0.4, 0.2));
                interPos = (mesh.vertices[triangleIndex[2]].position * 0.2f)
                    + (mesh.vertices[triangleIndex[1]].position + mesh.vertices[triangleIndex[0]].position) * 0.4f;
                drawLine(Ray { interPos, interNormal, 0.1 }, glm::abs(interNormal));

                interNormal = interpolateNormal(normals[0], normals[1], normals[2], glm::vec3(0.4, 0.2, 0.4));
                interPos = (mesh.vertices[triangleIndex[1]].position * 0.2f)
                    + (mesh.vertices[triangleIndex[0]].position + mesh.vertices[triangleIndex[2]].position) * 0.4f;
                drawLine(Ray { interPos, interNormal, 0.1 }, glm::abs(interNormal));

                interNormal = interpolateNormal(normals[0], normals[1], normals[2], glm::vec3(0.2, 0.4, 0.4));
                interPos = (mesh.vertices[triangleIndex[0]].position * 0.2f)
                    + (mesh.vertices[triangleIndex[1]].position + mesh.vertices[triangleIndex[2]].position) * 0.4f;
                drawLine(Ray { interPos, interNormal, 0.1 }, glm::abs(interNormal));
            }
        }
    }
}

void drawAA(
    const Scene& scene, const BvhInterface& bvh, const Plane& focalPlane, const glm::vec2& pixel,
    const Trackball& camera, const Screen& screen,
    const Features& features, const glm::mat2x3& apertureBasis
)
{
    if (!enableDebugDraw)
        return;

    if (!features.extra.enableMultipleRaysPerPixel) {
        Ray cameraRay = camera.generateRay(pixel);
        drawDOF(scene, bvh, focalPlane, cameraRay, features, apertureBasis);
        return;
    }

    glm::ivec2 windowResolution = screen.resolution();

    for (int i = 0; i < features.extra.AASamples; i++) {
        for (int j = 0; j < features.extra.AASamples; j++) {
            float subX = pixel.x + ((float)i + (float)rand() / (float)RAND_MAX) / (float)features.extra.AASamples;
            float subY = pixel.y + ((float)j + (float)rand() / (float)RAND_MAX) / (float)features.extra.AASamples;
            const glm::vec2 normalizedPixelPos {
                subX / float(windowResolution.x) * 2.0f - 1.0f,
                subY / float(windowResolution.y) * 2.0f - 1.0f,
            };
            Ray cameraRay = camera.generateRay(normalizedPixelPos);

            drawDOF(scene, bvh, focalPlane, cameraRay, features, apertureBasis);
        }
    }
}

void drawDOF(
    const Scene& scene, const BvhInterface& bvh, const Plane& focalPlane, const Ray& cameraRay,
    const Features& features, const glm::mat2x3& apertureBasis
)
{
    if (!enableDebugDraw)
        return;

    if (!features.extra.enableDepthOfField) {
        getFinalColor(scene, bvh, cameraRay, features);
        return;
    }
    

    uint64_t rand_state[4] { 4550963226, 4884, 2, 724 };
    glm::vec3 point = cameraRay.origin;
    int circleRes = 20;
    for (int i = 0; i < circleRes; i++) {
        float theta1 = 2 * std::numbers::pi * i / circleRes;
        float theta2 = 2 * std::numbers::pi * (i + 1) / circleRes;
        glm::vec3 p1
            = cameraRay.origin + features.extra.apertureRadius * apertureBasis * glm::vec2(cos(theta1), sin(theta1));
        glm::vec3 p2
            = cameraRay.origin + features.extra.apertureRadius * apertureBasis * glm::vec2(cos(theta2), sin(theta2));
        drawLine(p1, p2, glm::abs(glm::normalize(p2 - p1)));
    }

    float t = (focalPlane.D - glm::dot(focalPlane.normal, cameraRay.origin))
        / glm::dot(focalPlane.normal, cameraRay.direction);

    glm::vec3 pixelColor(0);
    for (int i = 0; i < features.extra.DOFSamples; i++) {
        float theta = to_double(next_rand(rand_state)) * 2 * std::numbers::pi;
        float r = std::sqrt(to_double(next_rand(rand_state)));
        glm::vec2 offset = glm::vec2(cos(theta), sin(theta)) * r * features.extra.apertureRadius;

        const Ray DOFRay {
            cameraRay.origin + apertureBasis * offset,
            glm::normalize((cameraRay.direction * t) - apertureBasis * offset),
            std::numeric_limits<float>::max(),
        };

        getFinalColor(scene, bvh, DOFRay, features);
    }
}