#pragma once

#include <vector>

/**
 * @brief Seam finding algorithms for content-aware image resizing
 */
namespace SeamCarving {

    /**
     * @brief Available seam finding algorithms
     */
    enum class Algorithm {
        GREEDY,     ///< Fast greedy approach (local optimum)
        DYNAMIC     ///< Optimal dynamic programming approach (global optimum)
    };

    /**
     * Find low energy vertical seam using greedy approach.
     * 
     * This function uses a greedy strategy that makes locally optimal choices
     * at each step. While faster than dynamic programming, it may not find
     * the globally optimal seam.
     * 
     * Time Complexity: O(width * height) - visits each pixel at most once during seam construction
     * Space Complexity: O(height) - stores only the seam path (one x-coordinate per row)
     * 
     * @param energy 2D matrix containing energy values for each pixel
     * @param width Image width
     * @param height Image height
     * @return Vector of x-coordinates defining the seam path
     */
    std::vector<int> find_low_energy_seam_greedy(
        const std::vector<std::vector<float>>& energy, 
        int width, 
        int height
    );

    /**
     * Find optimal vertical seam using dynamic programming approach.
     * 
     * This function finds the globally optimal vertical seam by computing the minimum
     * cumulative energy path from top to bottom using dynamic programming. Unlike the
     * greedy approach, this guarantees finding the seam with minimum total energy.
     * 
     * Time Complexity: O(width * height) - visits each pixel exactly once
     * Space Complexity: O(width * height) - stores DP table for cumulative energies
     * 
     * @param energy 2D matrix containing energy values for each pixel
     * @param width Image width
     * @param height Image height 
     * @return Vector of x-coordinates defining the optimal seam path
     */
    std::vector<int> find_low_energy_seam_dyn(
        const std::vector<std::vector<float>>& energy, 
        int width, 
        int height
    );

    /**
     * Find low energy vertical seam using the specified algorithm.
     * 
     * @param energy 2D matrix containing energy values for each pixel
     * @param width Image width
     * @param height Image height
     * @param algorithm Algorithm to use (GREEDY or DYNAMIC)
     * @return Vector of x-coordinates defining the seam path
     */
    std::vector<int> find_low_energy_seam(
        const std::vector<std::vector<float>>& energy, 
        int width, 
        int height,
        Algorithm algorithm = Algorithm::GREEDY
    );

    /**
     * Remove a vertical seam from the pixel array.
     * 
     * @param pixels Image pixel data (RGB format)
     * @param width Current image width
     * @param height Image height
     * @param channels Number of channels (should be 3 for RGB)
     * @param seam Vector of x-coordinates defining the seam to remove
     * @return New pixel array with seam removed
     */
    std::vector<unsigned char> remove_seam(
        const unsigned char* pixels,
        int width,
        int height,
        int channels,
        const std::vector<int>& seam
    );

    /**
     * Iteratively remove seams to reduce image width to target.
     * 
     * This function repeatedly finds and removes vertical seams until the image
     * reaches the specified target width. The energy map is recalculated after
     * each seam removal for optimal results.
     * 
     * @param pixels Image pixel data (RGB format)
     * @param original_width Original image width
     * @param height Image height
     * @param channels Number of channels (should be 3 for RGB)
     * @param target_width Desired final width
     * @param algorithm Algorithm to use for seam finding
     * @return Pair of (new pixel array, final width)
     */
    std::pair<std::vector<unsigned char>, int> reduce_width_iteratively(
        const unsigned char* pixels,
        int original_width,
        int height,
        int channels,
        int target_width,
        Algorithm algorithm = Algorithm::GREEDY
    );

    /**
     * Calculate energy map using gradient approach.
     * 
     * Uses a Sobel 3x3 operator to compute gradient magnitude as energy.
     * Higher energy indicates more important image features.
     * 
     * @param pixels Image pixel data (RGB format)
     * @param width Image width
     * @param height Image height
     * @param channels Number of channels (should be 3 for RGB)
     * @return 2D energy matrix
     */
    std::vector<std::vector<float>> calculate_energy(
        const unsigned char* pixels, 
        int width, 
        int height, 
        int channels
    );

} // namespace SeamCarving
