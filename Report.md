# CG Final Project Report

Kazek Ciaś 5467640
Renyi Yang 5470668
Toma Volentir 5454123
Group number: 62

## Work Distribution

Reflected in "final-project-workload-final.xlsx"


## Basic Features

### Shading

### Recursive ray-tracer

### Hard shadows

**Implementation**

A ray is shot from all intersection points to each point light; if it intersects geometry before the light the point
is marked as in shadow. Then the color determined by the shading function is multiplied by the average color
of all lights that reach the sampled point.

**Examples**

| ![Hard shadows - single light](./report/hard.bmp) | ![Hard shadows - two colored lights](./report/hardcolors.bmp) |
| ------------------------------------------------- | ------------------------------------------------------------- |
| _Single white light_                              | _Two colored lights_                                          |

### Area lights

**Implementation**

A number of samples are taken randomly from the light, using linear (for segment lights) or bilinear (for area lights)
interpolation to determine the color. Then the samples are checked for visibility (see [Hard Shadows](#hard-shadows))
and averaged together.

**Examples**

| ![Segment light](./report/segment.bmp) | ![Area light](./report/area.bmp) |
| -------------------------------------- | -------------------------------- |
| _Segment light_                        | _Area light_                     |

### BVH generation

The `Node` structure is designed according to the assignment requirement, I use 4 indexes to locate a triangle, `TriangleCoordinate = (meshIndex, vertexIndex1,vertexIndex2,vertexIndex3)`.when a bvh tree is created,  it goes through all the triangles and store the triangle coordinates in a vector, then it will be used in method `treeConstruction` which means to fill the global variable `tree`.

`treeConstruction` is a recursive function, at each layer the current node is generated according to the triangles it contains, then spilt the triangles in the middle and has the recursive calls for the next layer(if needed).

parameter `maxdepth` can be adjusted to control for the final shape of the `tree`, for default it's set to 10.

The effect of the BVH cannot be shown in the rendered image because it doesn't influence the appearance of the scene, but the improvement of the render speed is present in the "performance test" block.
| ![debug level](report\bvh_debug_level.png) | ![debug leaf](report\bvh_debug_leaf.png) |
| :----------------------------------------: | :--------------------------------------: |
|_AABBs for BVH nodes with level = 5_     |       _4th leaf of the cornell box_        |



### BVH traversal

### Normal interpolation

**Implementation**

Normal is calculated using barycentric coordinates obtained in intersection and later used in shading and reflections.

**Examples**

| ![Segment light](./report/nointerp.bmp) | ![Area light](./report/interp.bmp) |
| --------------------------------------- | ---------------------------------- |
| _Without interpolation_                 | _With interpolation_               |

### Texture

After the intersection point is generated, it's texture coordinates is calculated by interpolation, then the corresponding pixel is retrieved as a substitution for `hitInfo.material.kd`.

the debug ray will get color of the texture if texture mapping is enabled, we can also debug by checking the textured cube in ray-traced mode.
| ![front 3](./report/texture_front.bmp) | ![back 3](./report/texture_back.bmp) |
| --------------------------------------- | ---------------------------------- |
| _front 3 sufaces_                 | _back 3 surface_               |


---

## Extra Features

### Environment maps

Method `glm::vec3 acquireTexelEnvironment(const Image& image, const glm::vec3& direction, const Features& features)` is made to retrieve the corresponding pixel of the environmental texture given the direction of the camera.  A vector function is created to mapping normalized 3d vectors to a 2d vector (u,v) while u,v are values between 0 and 1. Then the uv coordinate is scaled according to the size of the texture and it's converted to the pixel index.

This method is called when the ray has no hit so it can create a background looking.
| ![without reflection](./report/monkey_in_lake.bmp) | ![with reflection](./report/cornell_box_in_lake.bmp) |
| --------------------------------------- | ---------------------------------- |
| _Without reflection_                 | _With reflection_               |

The debug ray is used for the visual debug, if it has no hit with the environmental mapping open, the ray supposed to have the color of the environmental texture. or it's red as default.
| ![no mapping](./report/env_debug_without.png) | ![with mapping](./report/env_debug.png) |
| --------------------------------------------- | --------------------------------------- |
| _Without environmental mapping_               | _With environmental mapping_            |


### SAH+binning

Now the splitting axis and boundary triangle are chosen base on the surface area heuristic. For each splitting the method `std::tuple<int, int> SurfaceAreaHeuristics(Scene* pScene,std::vector<glm::vec4>& traingleIndex)` is called, which will go through all the possible axis and bins, for each split choice, its score is calculated by $SurfaceAreaA*NumberofTrianglesA+SurfaceAreaB*NumberofTrianglesB$ (A and B  are AABBs of the splited node). And the final criteria is determined by the one that has the smallest score. All the rest part is identical to BVH.

`float getSurface(const AxisAlignedBox& boundary)`is made auxiliary for the score calculation.

The splitting plane can be drawn with AABBs, indicating where the node gets splited, for BVH all the planes towards the same direction and always near the middle of the AABB, but for SAH splitting planes vary a lot.
| ![BVH](./report/split_plane_without.png) | ![SAH](./report/split_plane_level5.png) |
| --------------------------------------------- | --------------------------------------- |
| _BVH splitting criteria_               | _SAH splitting criteria_            |

### Motion blur

The motion direction and motion speed can be configured via the GUI, the motion direction is considered as the same with its normalized value, and the motion speed is a relative value without physical meaning. During rendering, every ray's origin will have a offset within `(0, motionspeed•motion direction)`, for each ray, we take `numberofsample=200` offsets and the mean of the color those ray hits donates the final color of the pixel. 

| ![triangle blur](./report/blur0.bmp) | ![monkey blur](./report/motion2render.bmp) |
| --------------------------------------- | ---------------------------------- |
| _blured triangle(speed = 0.13) _                 | _blured monkey(speed = 0.2)_               |

For visual debug a line segment can be drawn whose direction represents the motion direction and the motion speed is reflected in the length of the segment(relative value).
| ![BVH](./report/trianglespeed0.13.png) | ![SAH](./report/motion2.png) |
| --------------------------------------------- | --------------------------------------- |
| _triangle motion_               | _monkey motion_            |


### Bloom filter

### Bilinear interpolation

### Mipmapping

### Irregular sampling

### Glossy reflections

**Implementation**

When taking reflection samples, instead of taking just one, a number (controlled by the user) of samples is taken,
and each sample's direction is offset by a uniformly distributed point on a circle perpendicular to the direction
with radius `1/shininess`. All samples with direction away from the mesh (`direction · normal > 0`) are then averaged
to determine the final result.

**Example**

![Gloss example](./report/gloss.bmp)

### Transparency

**Implementation**

When a sample ray hits a transparent surface, shading is calculated for the intersection point, a new sample is taken
in the same direction from the intersection and both results are combined using alpha blending.

This is done both for direct samples and shadow rays; `testVisibilityLightSample` has been modified to return
a `glm::vec3` indicating the colour of the shadow (which is later combined with the colour ofthe light).

Additionally, an option for translucency has been added: similar to [glossy reflections](#glossy-reflections),
instead of taking one sample when hitting a transparent surface, a number of randomly offset samples is taken.
The number of samples is controllable with a slider in the GUI.

**Examples**

For the following images the _Cornell Box_ scene was modified by replacing the small box's default material with this:

```mtl
newmtl shortBox
Ns 10.000002
Ka 1.000000 1.000000 1.000000
Kd 0.3000 0.710000 0.680000
Ks 0.000000 0.000000 0.000000
Ke 0.000000 0.000000 0.000000
Ni 1.000000
d 0.200000
illum 1
```

| ![Transparency - point light](./report/trans.bmp) | ![Transparency - area light](./report/transarea.bmp) | ![Translucency](./report/translucent.bmp) |
| ------------------------------------------------- | ---------------------------------------------------- | ----------------------------------------- |
| _Transparency with point light_                   | _Transparency with area light_                       | _Translucency_                            |

### Depth of field

**TODO - im tired now lol**

---

## Models

---

## Performance test

|                       | Cornell Box | Monkey   | Dragon    |
| --------------------- | ----------- | -------- | --------- |
| Num triangles         | 32          | 968      | 87K       |
| Time to create (BVH)  | 0.0571ms    | 1.8861ms | 219.089ms |
| Time to render (BVH)  | 7.98061s    | 9.55794s | 13.3239s  |
| Time to create (SAH)  | 0.1391ms    | 5.7154ms | 885.793ms |
| Time to render (SAH)  | 2.22238s    | 3.58297s | 10.8125s  |
| BVH levels            | 5           | 10       | 10        |
| Max tris p/ leaf node | 1           | 1        | 86        |

_*all the tests is measured within one execution, in the release mode_
