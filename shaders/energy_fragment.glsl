#version 330 core

// Input from vertex shader
in vec2 TexCoord;

// Output
out vec4 FragColor;

// Uniforms
uniform sampler2D inputTexture;
uniform vec2 texelSize;

void main() {
    vec2 tc = TexCoord;

    // Handle boundary conditions - set high energy for border pixels to discourage seams
    if (tc.x < texelSize.x || tc.x > 1.0 - texelSize.x ||
        tc.y < texelSize.y || tc.y > 1.0 - texelSize.y) {
        FragColor = vec4(1000.0, 0.0, 0.0, 1.0);
        return;
    }

    // Sample the 3x3 neighborhood and convert to grayscale using luminance weights
    float tl = dot(texture(inputTexture, tc + vec2(-texelSize.x, -texelSize.y)).rgb, vec3(0.299, 0.587, 0.114));
    float tm = dot(texture(inputTexture, tc + vec2(0.0, -texelSize.y)).rgb, vec3(0.299, 0.587, 0.114));
    float tr = dot(texture(inputTexture, tc + vec2(texelSize.x, -texelSize.y)).rgb, vec3(0.299, 0.587, 0.114));

    float ml = dot(texture(inputTexture, tc + vec2(-texelSize.x, 0.0)).rgb, vec3(0.299, 0.587, 0.114));
    float mr = dot(texture(inputTexture, tc + vec2(texelSize.x, 0.0)).rgb, vec3(0.299, 0.587, 0.114));

    float bl = dot(texture(inputTexture, tc + vec2(-texelSize.x, texelSize.y)).rgb, vec3(0.299, 0.587, 0.114));
    float bm = dot(texture(inputTexture, tc + vec2(0.0, texelSize.y)).rgb, vec3(0.299, 0.587, 0.114));
    float br = dot(texture(inputTexture, tc + vec2(texelSize.x, texelSize.y)).rgb, vec3(0.299, 0.587, 0.114));

    // Apply Sobel operators
    // Sobel X kernel: [-1, 0, 1; -2, 0, 2; -1, 0, 1]
    float gx = (-1.0 * tl) + (0.0 * tm) + (1.0 * tr) +
               (-2.0 * ml) + (0.0 * 0.0) + (2.0 * mr) +
               (-1.0 * bl) + (0.0 * bm) + (1.0 * br);

    // Sobel Y kernel: [-1, -2, -1; 0, 0, 0; 1, 2, 1]
    float gy = (-1.0 * tl) + (-2.0 * tm) + (-1.0 * tr) +
               ( 0.0 * ml) + ( 0.0 * 0.0) + ( 0.0 * mr) +
               ( 1.0 * bl) + ( 2.0 * bm) + ( 1.0 * br);

    // Calculate gradient magnitude as energy
    float energy = sqrt(gx * gx + gy * gy);

    // Store energy in red channel
    FragColor = vec4(energy, 0.0, 0.0, 1.0);
}
