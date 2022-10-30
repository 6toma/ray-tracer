#pragma once
#include "common.h"
#include <array>
#include <framework/ray.h>
#include <vector>

// Forward declaration.
struct Scene;

struct Node {
    // represent the level of the node in the tree, the depth of the root is 0
    int depth;
    // true iff the node doesn't have children
    bool isleaf;
    // if this node is an internal node, it stores the TWO indexes of it's children. if this node only has one child,
    // the other index will be set as -1. if this node is a leaf node, it stores infomation about traingles in it's
    // AABB,every continuous 4 values represent a triangle so the length of the vector should always be a mutiple of
    // four. let's say (a,b,c,d) stand for the coordinate of a triangle, "a" represents the ordinal of it's
    //cooresponding mesh, start from zero, (b,c,d) are indexes of points stored in this mesh, representing 3 vertices
    // for the triangle. see method "debugDrawLeaf" for an usage example
    std::vector<int> children;
    // represent the bounding box, see method "debugDrawLevel" for an usage exmaple.
    AxisAlignedBox boundary;
    /*
    Node(std::vector<glm::vec4>& index) {
        traingleIndex = index;
        boundary.lower = glm::vec3{ INT_MAX };
        boundary.upper = glm::vec3 { INT_MIN };
        for (auto t : traingleIndex) {
            Vertex v1 = pScene->meshes[x[0]].vertices[x[1]];
            Vertex v2 = pScene->meshes[x[0]].vertices[x[2]];
            Vertex v3 = pScene->meshes[x[0]].vertices[x[3]];
        }
    }*/
};

class BoundingVolumeHierarchy {
public:
    // Constructor. Receives the scene and builds the bounding volume hierarchy.
    BoundingVolumeHierarchy(Scene* pScene);

    // Return how many levels there are in the tree that you have constructed.
    [[nodiscard]] int numLevels() const;

    // Return how many leaf nodes there are in the tree that you have constructed.
    [[nodiscard]] int numLeaves() const;

    // Visual Debug 1: Draw the bounding boxes of the nodes at the selected level.
    void debugDrawLevel(int level);

    // Visual Debug 2: Draw the triangles of the i-th leaf
    void debugDrawLeaf(int leafIdx);

    // Return true if something is hit, returns false otherwise.
    // Only find hits if they are closer than t stored in the ray and the intersection
    // is on the correct side of the origin (the new t >= 0).
    bool intersect(Ray& ray, HitInfo& hitInfo, const Features& features) const;


private:
    int m_numLevels;
    int m_numLeaves;
    Scene* m_pScene;
};