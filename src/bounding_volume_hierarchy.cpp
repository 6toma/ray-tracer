#include "bounding_volume_hierarchy.h"
#include "draw.h"
#include "intersect.h"
#include "scene.h"
#include "texture.h"
#include "interpolate.h"
#include "common.h"
#include <glm/glm.hpp>
#include <chrono>
#include<iostream>
#include <queue>

//!!! the final tree stucture, all the access to bvh should be via this.
std::vector<Node> tree;
void updateAABB(AxisAlignedBox &boundary, Vertex &v1, Vertex &v2, Vertex &v3)
{
    for (int i = 0; i < 3; i++) { 
        boundary.lower[i]
            = std::min(std::min(boundary.lower[i], v1.position[i]), std::min(v2.position[i], v3.position[i]));
        boundary.upper[i]
            = std::max(std::max(boundary.upper[i], v1.position[i]), std::max(v2.position[i], v3.position[i]));
    }
}
float getSurface(const AxisAlignedBox& boundary) { 
    glm::vec3 t = boundary.upper - boundary.lower;
    return 2.0 * (t.x * t.y + t.x * t.z + t.y * t.z);
}
std::tuple<int, int> SurfaceAreaHeuristics(Scene* pScene,std::vector<glm::vec4>& traingleIndex)
{
    int axis, optAxis,optIndex = -1;
    long double optScore = 1e36;
    int n = traingleIndex.size();
    std::vector<float> l;
    std::vector<float> r;
    for (axis = 0; axis < 3; axis++) {
        l.clear(), r.clear();
        AxisAlignedBox leftBoundary,rightBoundary;
        leftBoundary.lower = glm::vec3 { 1e9 };
        leftBoundary.upper = glm::vec3 { -1e9 };
        rightBoundary.lower = glm::vec3  { 1e9 };
        rightBoundary.upper = glm::vec3 { -1e9 };
        std::sort(traingleIndex.begin(), traingleIndex.end(), [&](const auto& x, const auto& y) {
            Vertex v1 = pScene->meshes[x[0]].vertices[x[1]];
            Vertex v2 = pScene->meshes[x[0]].vertices[x[2]];
            Vertex v3 = pScene->meshes[x[0]].vertices[x[3]];
            Vertex v4 = pScene->meshes[y[0]].vertices[y[1]];
            Vertex v5 = pScene->meshes[y[0]].vertices[y[2]];
            Vertex v6 = pScene->meshes[y[0]].vertices[y[3]];
            return v1.position[axis] + v2.position[axis] + v3.position[axis]
                < v4.position[axis] + v5.position[axis] + v6.position[axis];
        });
        for (int leftAmount = 1; leftAmount < n; leftAmount++) { 
            glm::vec4 front = traingleIndex[leftAmount-1];
            glm::vec4 back = traingleIndex[n - leftAmount];
            updateAABB(
                leftBoundary, pScene->meshes[front[0]].vertices[front[1]], pScene->meshes[front[0]].vertices[front[2]],
                pScene->meshes[front[0]].vertices[front[3]]
            );
            updateAABB(
                rightBoundary, pScene->meshes[back[0]].vertices[back[1]], pScene->meshes[back[0]].vertices[back[2]],
                pScene->meshes[back[0]].vertices[back[3]]
            );
            l.push_back(getSurface(leftBoundary));
            r.push_back(getSurface(rightBoundary));
        }
        for (int leftAmount = 1; leftAmount < n; leftAmount++) {
            float s1 = l[leftAmount - 1];
            float s2 = r[n - 1 - leftAmount];
            float cur = s1 * leftAmount + s2 * (n - leftAmount);
        //    std::cout << s1 << " " << s2 << " " << leftAmount << " " << cur << " " << optScore
        //              << "\n ";
            if (cur < optScore) {
                optAxis = axis;
                optIndex = leftAmount;
                optScore = cur;
            }
        }

    }
    return std::make_tuple(optAxis,optIndex);
}
int treeConstruction(
    std::vector<Node>& t, Scene* pScene, std::vector<glm::vec4>& traingleIndex, int depth,
    const Features& features, int maxdepth = 10)
{
    if (depth > maxdepth||traingleIndex.empty())
        return -1;
    Node node;
    node.boundary.lower = glm::vec3 { 1e9 };
    node.boundary.upper = glm::vec3 { -1e9 };
    for (auto t : traingleIndex) {
        Vertex v1 = pScene->meshes[t[0]].vertices[t[1]];
        Vertex v2 = pScene->meshes[t[0]].vertices[t[2]];
        Vertex v3 = pScene->meshes[t[0]].vertices[t[3]];
        updateAABB(node.boundary, v1, v2, v3);
    }
    node.depth = depth;
    node.isleaf = false;
    t.push_back(node);
    int res = t.size() - 1;
    if (traingleIndex.size() == 1) {
        t[res].isleaf = true;
        for (auto traingle : traingleIndex) {
            for (int i = 0; i < 4; i++)
                t[res].children.push_back(traingle[i]);
        }
        return res;
    }
    int median = traingleIndex.size() / 2;
    int axis = depth % 3;
    if (features.extra.enableBvhSahBinning)
        std::tie(axis, median) = SurfaceAreaHeuristics(pScene,traingleIndex);
    std::sort(
        traingleIndex.begin(),
        traingleIndex.end(),
        [&](const auto& x, const auto& y) {
            Vertex v1 = pScene->meshes[x[0]].vertices[x[1]];
            Vertex v2 = pScene->meshes[x[0]].vertices[x[2]];
            Vertex v3 = pScene->meshes[x[0]].vertices[x[3]];
            Vertex v4 = pScene->meshes[y[0]].vertices[y[1]];
            Vertex v5 = pScene->meshes[y[0]].vertices[y[2]];
            Vertex v6 = pScene->meshes[y[0]].vertices[y[3]];
            return v1.position[axis] + v2.position[axis] + v3.position[axis] < v4.position[axis] + v5.position[axis] + v6.position[axis];
        });
    std::vector<glm::vec4> leftcontent = { traingleIndex.begin(), traingleIndex.begin() + median };
    std::vector<glm::vec4> rightcontent = { traingleIndex.begin() + median, traingleIndex.end() };
    int v1 = treeConstruction(t, pScene, leftcontent, depth + 1,features,maxdepth);
    t[res].children.push_back(v1);
    int v2 = treeConstruction(t, pScene, rightcontent, depth + 1,features,maxdepth);
    t[res].children.push_back(v2);
    if (t[res].children[0] == -1 && t[res].children[1] == -1) {
        t[res].children.clear();
        t[res].isleaf = true;
        for (auto traingle : traingleIndex) {
            for (int i = 0; i < 4; i++)
                t[res].children.push_back(traingle[i]);
        }
    }
    return res;
}
BoundingVolumeHierarchy::BoundingVolumeHierarchy(Scene* pScene, const Features& features)
    : m_pScene(pScene)
{
    using clock = std::chrono::high_resolution_clock;
    const auto start = clock::now();
    tree.clear();
    std::vector<glm::vec4> traingleIndex;
    int ind = 0;
    for (auto m : pScene->meshes) {
        for (auto v : m.triangles) {
            traingleIndex.push_back(glm::vec4 { ind, v.x, v.y, v.z });
        }
        ind++;
    }
    treeConstruction(tree, pScene,traingleIndex,0,features);
    const auto end = clock::now();
    std::cout << "Time to create bvh: " << std::chrono::duration<float, std::milli>(end - start).count()
              << " milliseconds" << std::endl;
}

// Return the depth of the tree that you constructed. This is used to tell the
// slider in the UI how many steps it should display for Visual Debug 1.
int BoundingVolumeHierarchy::numLevels() const
{
    int ans = 0;
    for (Node node : tree) {
        ans = std::max(ans, node.depth);
    }
    return ans+1;
}

// Return the number of leaf nodes in the tree that you constructed. This is used to tell the
// slider in the UI how many steps it should display for Visual Debug 2.
int BoundingVolumeHierarchy::numLeaves() const
{
    int ans = 0;
    for (Node node : tree) {
        if (node.isleaf)
            ans++;
    }
    return ans;
}

// Use this function to visualize your BVH. This is useful for debugging. Use the functions in
// draw.h to draw the various shapes. We have extended the AABB draw functions to support wireframe
// mode, arbitrary colors and transparency.
void BoundingVolumeHierarchy::debugDrawLevel(int level)
{
    // Draw the AABB as a transparent green box.
    //AxisAlignedBox aabb{ glm::vec3(-0.05f), glm::vec3(0.05f, 1.05f, 1.05f) };
    //drawShape(aabb, DrawMode::Filled, glm::vec3(0.0f, 1.0f, 0.0f), 0.2f);
    for (Node node : tree) {
        if (node.depth == level) {
            drawAABB(node.boundary, DrawMode::Wireframe, glm::vec3(1.0f, 1.0f, 1.0f), 0.7f);
        }
    }
    // Draw the AABB as a (white) wireframe box.
  //  AxisAlignedBox aabb { glm::vec3(0.0f), glm::vec3(0.0f, 1.05f, 1.05f) };
    //drawAABB(aabb, DrawMode::Wireframe);
  //  drawAABB(aabb, DrawMode::Filled, glm::vec3(0.05f, 1.0f, 0.05f), 0.1f);
}


// Use this function to visualize your leaf nodes. This is useful for debugging. The function
// receives the leaf node to be draw (think of the ith leaf node). Draw the AABB of the leaf node and all contained triangles.
// You can draw the triangles with different colors. NoteL leafIdx is not the index in the node vector, it is the
// i-th leaf node in the vector.
void BoundingVolumeHierarchy::debugDrawLeaf(int leafIdx)
{
    // Draw the AABB as a transparent green box.
    //AxisAlignedBox aabb{ glm::vec3(-0.05f), glm::vec3(0.05f, 1.05f, 1.05f) };
    //drawShape(aabb, DrawMode::Filled, glm::vec3(0.0f, 1.0f, 0.0f), 0.2f);

    // Draw the AABB as a (white) wireframe box.
    //AxisAlignedBox aabb { glm::vec3(0.0f), glm::vec3(0.0f, 1.05f, 1.05f) };
    //drawAABB(aabb, DrawMode::Wireframe);
    int ind = 0;
    for (Node node : tree) {
        if (node.isleaf) {
            if (ind == leafIdx) {
                drawAABB(node.boundary, DrawMode::Wireframe, glm::vec3(1.0f, 1.0f, 1.0f), 0.7f);
                for (int i = 0; i < node.children.size();i+=4) {
                    int tt = node.children[i];
                    int x = node.children[i + 1];
                    int y = node.children[i + 2];
                    int z = node.children[i + 3];
                    Vertex v1 = m_pScene->meshes[tt].vertices[x];
                    Vertex v2 = m_pScene->meshes[tt].vertices[y];
                    Vertex v3 = m_pScene->meshes[tt].vertices[z];
                    drawTriangle(v1, v2, v3);
                }
                return;
            }
            ind++;
        }
    }

    // once you find the leaf node, you can use the function drawTriangle (from draw.h) to draw the contained primitives
}

// Return true if something is hit, returns false otherwise. Only find hits if they are closer than t stored
// in the ray and if the intersection is on the correct side of the origin (the new t >= 0). Replace the code
// by a bounding volume hierarchy acceleration structure as described in the assignment. You can change any
// file you like, including bounding_volume_hierarchy.h.
bool BoundingVolumeHierarchy::intersect(Ray& ray, HitInfo& hitInfo, const Features& features) const
{
    // If BVH is not enabled, use the naive implementation.
    if (!features.enableAccelStructure) {
        bool hit = false;
        // Intersect with all triangles of all meshes.
        for (const auto& mesh : m_pScene->meshes) {
            for (const auto& tri : mesh.triangles) {
                const auto v0 = mesh.vertices[tri[0]];
                const auto v1 = mesh.vertices[tri[1]];
                const auto v2 = mesh.vertices[tri[2]];
                bool h = intersectRayWithTriangle(v0.position, v1.position, v2.position, ray, hitInfo);
                if (h) {
                    if (features.enableNormalInterp) {
                        hitInfo.normal = interpolateNormal(v0.normal, v1.normal, v2.normal, hitInfo.barycentricCoord);
                    }
                    if (features.enableTextureMapping) {
                        hitInfo.texCoord = interpolateTexCoord(v0.normal, v1.normal, v2.normal, hitInfo.barycentricCoord);
                    }
                    hitInfo.material = mesh.material;
                    hit = true;
                }
            }
        }
        // Intersect with spheres.
        for (const auto& sphere : m_pScene->spheres)
            hit |= intersectRayWithShape(sphere, ray, hitInfo);
        return hit;
    } else {
        bool hit = false;
        float rayT = std::numeric_limits<float>::max();
        std::queue<Node> q;
        q.push(tree[0]); // push root to queue

        while (!q.empty()) {
            Node current = q.front();
            q.pop();
            if (current.isleaf) {
                for (int i = 0; i < current.children.size(); i += 4) {
                    int tt = current.children[i];
                    int x = current.children[i + 1];
                    int y = current.children[i + 2];
                    int z = current.children[i + 3];
                    Vertex v1 = m_pScene->meshes[tt].vertices[x];
                    Vertex v2 = m_pScene->meshes[tt].vertices[y];
                    Vertex v3 = m_pScene->meshes[tt].vertices[z];
                    bool h = intersectRayWithTriangle(v1.position, v2.position, v3.position, ray, hitInfo);
                    if (h && ray.t >= 0 && ray.t < rayT) {
                        drawTriangle(v1, v2, v3);
                        rayT = ray.t;
                        hitInfo.material = m_pScene->meshes[tt].material;
                        if (features.enableNormalInterp) {
                            hitInfo.normal = interpolateNormal(v1.normal, v2.normal, v3.normal, hitInfo.barycentricCoord);
                        }
                        if (features.enableTextureMapping) {
                            hitInfo.texCoord = interpolateTexCoord(v1.normal, v2.normal, v3.normal, hitInfo.barycentricCoord);
                        }
                        hit = true;
                    }
                }
            } else {
                float t = ray.t;
                if (intersectRayWithShape(current.boundary, ray)) {
                    drawAABB(current.boundary, DrawMode::Wireframe, glm::vec3(1.0f, 1.0f, 1.0f), 0.7f);
                    if (ray.t > t) {
                        ray.t = t;
                        continue;
                    }
                    ray.t = t;
                    for (int i = 0; i < current.children.size(); i++)
                        if (current.children[i] != -1
                            && intersectRayWithShape(tree[current.children[i]].boundary, ray)) {
                            drawAABB(tree[current.children[i]].boundary, DrawMode::Wireframe, glm::vec3(1.0f, 1.0f, 1.0f), 0.7f);
                            ray.t = t;
                            q.push(tree[current.children[i]]);
                        }
                }
            }
        }
        return hit;
    }
}