#version 330 core

// Input vertex attributes
layout (location = 0) in vec2 position;
layout (location = 1) in vec2 texCoord;

// Output to fragment shader
out vec2 TexCoord;

void main() {
    TexCoord = texCoord;
    gl_Position = vec4(position, 0.0, 1.0);
}
