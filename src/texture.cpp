#include "texture.h"
#include <cmath>
#include <framework/image.h>
#include <iostream>
#include <numbers>
static const std::filesystem::path dataDirPath { DATA_DIR };
glm::vec3 acquireTexel(const Image& image, const glm::vec2& texCoord, const Features& features)
{
    // TODO: implement this function.
    // Given texcoords, return the corresponding pixel of the image
    // The pixel are stored in a 1D array of row major order
    // you can convert from position (i,j) to an index using the method seen in the lecture
    // Note, the center of the first pixel is at image coordinates (0.5, 0.5)
    return image.pixels[0];
}
glm::vec3 acquireTexelEnvironment(const Image& image, const glm::vec3& direction, const Features& features)
{
    float v, u;
    if (!features.extra.enableEnvironmentMapping)
        return glm::vec3 { 0.0 };
    if (abs(direction.x) < 1e-6) {
        if (direction.y > 0)
            v = 0.75;
        else
            v = 0.25;
    } else
        v = std::atan2l((long double)direction.y, (long double)direction.x) / 2.0 / std::numbers::pi + 0.5;
    if (abs(direction.z + 1) < 1e-6)
        u = 0;
    else if (abs(direction.z - 1) < 1e-6)
        u = 1;
    else
        u = std::asin(direction.z) / std::numbers::pi + 0.5;
    int j = floor(u * (image.width - 1) + 0.5);
    int i = image.height - floor(v * (image.height - 1) + 0.5) - 1;
    /*
    if (i < 0 || i >= image.height || j < 0 || j >= image.width)
        std::cout << u << " " << v << " " << i << " " << j << " " << image.width << " " << image.height
                  << " " <<direction.x << " " << direction.y << " " << direction.y / direction.x << " "
                  << std::atan2l((long double)direction.y, (long double)direction.x) << " " << direction.z << " "
                  << std::asin(direction.z) << " " << abs(direction.z + 1)
                  << "\n ";*/
    return image.pixels[i * image.width + j];
}