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

} // namespace SeamCarving
