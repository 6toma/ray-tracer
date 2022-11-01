# CG Final Project Report

Kazek Ciaś 5467640
Renyi Yang 5470668
Toma Volentir 5454123
Group number: 62

---

## Work Distribution

Reflected in "final-project-workload-final.xlsx"

---

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

### BVH traversal

### Normal interpolation

**Implementation**

Normal is calculated using barycentric coordinates obtained in intersection and later used in shading and reflections.

**Examples**

| ![Segment light](./report/nointerp.bmp) | ![Area light](./report/interp.bmp) |
| --------------------------------------- | ---------------------------------- |
| _Without interpolation_                 | _With interpolation_               |

### Texture

---

## Extra Features

### Environment maps

### SAH+binning

### Motion blur

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

|                       | Cornell Box | Monkey | Dragon |
| --------------------- | ----------- | ------ | :----: |
| Num triangles         |             |        |        |
| Time to create (BVH)  |             |        |        |
| Time to render (BVH)  |             |        |        |
| Time to create (SAH)  |             |        |        |
| Time to render (SAH)  |             |        |        |
| BVH levels            |             |        |        |
| Max tris p/ leaf node |             |        |        |
