#include "texture.h"
#include <framework/image.h>
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
    float PI = 3.14159265358979323846;
    if (!features.extra.enableEnvironmentMapping)
        return glm::vec3 { 0.0 };
    float v = std::atan2(direction.y , direction.x) / 2/PI + 0.5;
    float u = std::asin(direction.z) / PI + 0.5;
    int j = floor(u * (image.width - 1) + 0.5);
    int i = image.height  - floor(v * (image.height - 1) + 0.5);
    return image.pixels[i * image.width + j];
}