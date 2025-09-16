#pragma once

#include <vector>
#include <glad/glad.h>

namespace GPUEnergy {
    
    /**
     * @brief Initialize GPU compute shaders for energy calculation
     * @return true if initialization successful, false otherwise
     */
    bool initialize();
    
    /**
     * @brief Cleanup GPU resources
     */
    void cleanup();
    
    /**
     * @brief Calculate energy map using GPU compute shader (Sobel operator)
     * @param pixels Image pixel data (RGB format)
     * @param width Image width
     * @param height Image height
     * @param channels Number of channels (should be 3 for RGB)
     * @return 2D energy matrix
     */
    std::vector<std::vector<float>> calculate_energy_gpu(
        const unsigned char* pixels, 
        int width, 
        int height, 
        int channels
    );
    
} // namespace GPUEnergy
