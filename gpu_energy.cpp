#include "gpu_energy.h"
#include <spdlog/spdlog.h>
#include <fstream>
#include <sstream>
#include <vector>
#include <algorithm>

namespace GPUEnergy {

static GLuint shader_program = 0;
static GLuint input_texture = 0;
static GLuint framebuffer = 0;
static GLuint energy_texture = 0;
static GLuint quad_vao = 0;
static GLuint quad_vbo = 0;
static bool initialized = false;

// Load and compile shader from file
static std::string load_shader_source(const std::string& filepath) {
    std::ifstream file(filepath);
    if (!file.is_open()) {
        spdlog::error("Failed to open shader file: {}", filepath);
        return "";
    }
    
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

// Compile shader
static GLuint compile_shader(const std::string& source, GLenum shader_type) {
    GLuint shader = glCreateShader(shader_type);
    const char* source_cstr = source.c_str();
    glShaderSource(shader, 1, &source_cstr, nullptr);
    glCompileShader(shader);
    
    GLint success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        GLchar info_log[512];
        glGetShaderInfoLog(shader, 512, nullptr, info_log);
        const char* shader_type_str = (shader_type == GL_VERTEX_SHADER) ? "vertex" : "fragment";
        spdlog::error("{} shader compilation failed: {}", shader_type_str, info_log);
        glDeleteShader(shader);
        return 0;
    }
    
    return shader;
}

// Create shader program
static GLuint create_shader_program(GLuint vertex_shader, GLuint fragment_shader) {
    GLuint program = glCreateProgram();
    glAttachShader(program, vertex_shader);
    glAttachShader(program, fragment_shader);
    glLinkProgram(program);
    
    GLint success;
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success) {
        GLchar info_log[512];
        glGetProgramInfoLog(program, 512, nullptr, info_log);
        spdlog::error("Shader program linking failed: {}", info_log);
        glDeleteProgram(program);
        return 0;
    }
    
    return program;
}

// Create fullscreen quad for rendering
static void create_quad() {
    // Fullscreen quad vertices (position + texcoord)
    float quad_vertices[] = {
        // positions   // texCoords
        -1.0f, -1.0f,  0.0f, 0.0f,
         1.0f, -1.0f,  1.0f, 0.0f,
         1.0f,  1.0f,  1.0f, 1.0f,
        -1.0f, -1.0f,  0.0f, 0.0f,
         1.0f,  1.0f,  1.0f, 1.0f,
        -1.0f,  1.0f,  0.0f, 1.0f
    };
    
    glGenVertexArrays(1, &quad_vao);
    glGenBuffers(1, &quad_vbo);
    
    glBindVertexArray(quad_vao);
    glBindBuffer(GL_ARRAY_BUFFER, quad_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quad_vertices), quad_vertices, GL_STATIC_DRAW);
    
    // Position attribute
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    
    // Texture coordinate attribute
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);
    
    glBindVertexArray(0);
}

bool initialize() {
    if (initialized) {
        return true;
    }
    
    // Check OpenGL version - we need at least 3.2 for macOS compatibility
    if (!GLAD_GL_VERSION_3_2) {
        spdlog::error("OpenGL 3.2 required for GPU energy calculation");
        return false;
    }
    
    // Load and compile vertex shader
    std::string vertex_source = load_shader_source("shaders/energy_vertex.glsl");
    if (vertex_source.empty()) {
        return false;
    }
    
    GLuint vertex_shader = compile_shader(vertex_source, GL_VERTEX_SHADER);
    if (vertex_shader == 0) {
        return false;
    }
    
    // Load and compile fragment shader
    std::string fragment_source = load_shader_source("shaders/energy_fragment.glsl");
    if (fragment_source.empty()) {
        glDeleteShader(vertex_shader);
        return false;
    }
    
    GLuint fragment_shader = compile_shader(fragment_source, GL_FRAGMENT_SHADER);
    if (fragment_shader == 0) {
        glDeleteShader(vertex_shader);
        return false;
    }
    
    // Create shader program
    shader_program = create_shader_program(vertex_shader, fragment_shader);
    glDeleteShader(vertex_shader);
    glDeleteShader(fragment_shader);
    
    if (shader_program == 0) {
        return false;
    }
    
    // Create fullscreen quad
    create_quad();
    
    // Generate textures and framebuffer
    glGenTextures(1, &input_texture);
    glGenTextures(1, &energy_texture);
    glGenFramebuffers(1, &framebuffer);
    
    // Set up shader uniforms
    glUseProgram(shader_program);
    GLint input_texture_loc = glGetUniformLocation(shader_program, "inputTexture");
    GLint texel_size_loc = glGetUniformLocation(shader_program, "texelSize");
    
    if (input_texture_loc == -1 || texel_size_loc == -1) {
        spdlog::error("Failed to get shader uniform locations");
        return false;
    }
    
    glUniform1i(input_texture_loc, 0); // Texture unit 0
    
    initialized = true;
    spdlog::info("GPU energy calculation initialized successfully (render-to-texture)");
    return true;
}

void cleanup() {
    if (!initialized) {
        return;
    }
    
    if (shader_program != 0) {
        glDeleteProgram(shader_program);
        shader_program = 0;
    }
    
    if (input_texture != 0) {
        glDeleteTextures(1, &input_texture);
        input_texture = 0;
    }
    
    if (energy_texture != 0) {
        glDeleteTextures(1, &energy_texture);
        energy_texture = 0;
    }
    
    if (framebuffer != 0) {
        glDeleteFramebuffers(1, &framebuffer);
        framebuffer = 0;
    }
    
    if (quad_vao != 0) {
        glDeleteVertexArrays(1, &quad_vao);
        quad_vao = 0;
    }
    
    if (quad_vbo != 0) {
        glDeleteBuffers(1, &quad_vbo);
        quad_vbo = 0;
    }
    
    initialized = false;
    spdlog::info("GPU energy calculation cleanup completed");
}

std::vector<std::vector<float>> calculate_energy_gpu(
    const unsigned char* pixels, 
    int width, 
    int height, 
    int channels
) {
    if (!initialized) {
        spdlog::error("GPU energy calculation not initialized");
        return {};
    }
    
    if (channels != 3) {
        spdlog::error("GPU energy calculation only supports RGB images (3 channels)");
        return {};
    }
    
    // Save current OpenGL state
    GLint prev_framebuffer;
    GLint prev_viewport[4];
    glGetIntegerv(GL_FRAMEBUFFER_BINDING, &prev_framebuffer);
    glGetIntegerv(GL_VIEWPORT, prev_viewport);
    
    // Upload input image to texture
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, input_texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, pixels);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    
    // Create energy output texture
    glBindTexture(GL_TEXTURE_2D, energy_texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, width, height, 0, GL_RGBA, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    
    // Set up framebuffer
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, energy_texture, 0);
    
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        spdlog::error("Framebuffer not complete");
        // Restore state
        glBindFramebuffer(GL_FRAMEBUFFER, prev_framebuffer);
        glViewport(prev_viewport[0], prev_viewport[1], prev_viewport[2], prev_viewport[3]);
        return {};
    }
    
    // Set viewport and render
    glViewport(0, 0, width, height);
    glUseProgram(shader_program);
    
    // Set uniforms
    GLint texel_size_loc = glGetUniformLocation(shader_program, "texelSize");
    glUniform2f(texel_size_loc, 1.0f / width, 1.0f / height);
    
    // Bind input texture
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, input_texture);
    
    // Render fullscreen quad
    glBindVertexArray(quad_vao);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);
    
    // Read back results from GPU
    std::vector<float> energy_data(width * height * 4); // RGBA format
    glReadPixels(0, 0, width, height, GL_RGBA, GL_FLOAT, energy_data.data());
    
    // Restore OpenGL state
    glBindFramebuffer(GL_FRAMEBUFFER, prev_framebuffer);
    glViewport(prev_viewport[0], prev_viewport[1], prev_viewport[2], prev_viewport[3]);
    
    // Convert to 2D vector format expected by seam carving algorithm
    // Energy is stored in the red channel
    std::vector<std::vector<float>> energy_matrix(height, std::vector<float>(width));
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            int pixel_index = (y * width + x) * 4; // RGBA format
            energy_matrix[y][x] = energy_data[pixel_index]; // Red channel contains energy
        }
    }
    
    spdlog::debug("GPU energy calculation completed: {}x{}", width, height);
    return energy_matrix;
}

} // namespace GPUEnergy
