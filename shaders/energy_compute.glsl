#version 430

layout(local_size_x = 16, local_size_y = 16) in;

// Input image as a texture
layout(binding = 0, rgba8) uniform restrict readonly image2D input_image;

// Output energy map
layout(binding = 1, r32f) uniform restrict writeonly image2D energy_output;

// Sobel X kernel
const mat3 sobel_x = mat3(
    -1.0, 0.0, 1.0,
    -2.0, 0.0, 2.0,
    -1.0, 0.0, 1.0
);

// Sobel Y kernel
const mat3 sobel_y = mat3(
    -1.0, -2.0, -1.0,
     0.0,  0.0,  0.0,
     1.0,  2.0,  1.0
);

void main() {
    ivec2 pixel_coord = ivec2(gl_GlobalInvocationID.xy);
    ivec2 image_size = imageSize(input_image);

    // Boundary check
    if (pixel_coord.x >= image_size.x || pixel_coord.y >= image_size.y) {
        return;
    }

    // Handle border pixels - set to high energy to discourage seams at borders
    if (pixel_coord.x == 0 || pixel_coord.y == 0 ||
        pixel_coord.x == image_size.x - 1 || pixel_coord.y == image_size.y - 1) {
        imageStore(energy_output, pixel_coord, vec4(1000.0, 0.0, 0.0, 0.0));
        return;
    }

    float gradient_x = 0.0;
    float gradient_y = 0.0;

    // Apply Sobel operator by sampling 3x3 neighborhood
    for (int dy = -1; dy <= 1; dy++) {
        for (int dx = -1; dx <= 1; dx++) {
            ivec2 sample_coord = pixel_coord + ivec2(dx, dy);

            // Get RGB values and convert to grayscale (luminance)
            vec4 sample_color = imageLoad(input_image, sample_coord);
            float luminance = 0.299 * sample_color.r + 0.587 * sample_color.g + 0.114 * sample_color.b;

            // Accumulate gradients using Sobel kernels
            gradient_x += luminance * sobel_x[dy + 1][dx + 1];
            gradient_y += luminance * sobel_y[dy + 1][dx + 1];
        }
    }

    // Compute gradient magnitude as energy
    float energy = sqrt(gradient_x * gradient_x + gradient_y * gradient_y);

    // Store result
    imageStore(energy_output, pixel_coord, vec4(energy, 0.0, 0.0, 0.0));
}
