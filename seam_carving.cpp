#include "seam_carving.h"
#include <limits>
#include <algorithm>

namespace SeamCarving {

std::vector<int> find_low_energy_seam_greedy(
    const std::vector<std::vector<float>>& energy, 
    int width, 
    int height
) {
    std::vector<int> seam(height);
    
    // Start from the top row - find pixel with minimum energy
    int min_x = 0;
    float min_energy = energy[0][0];
    for (int x = 1; x < width; x++) {
        if (energy[0][x] < min_energy) {
            min_energy = energy[0][x];
            min_x = x;
        }
    }
    seam[0] = min_x;
    
    // For each subsequent row, choose the neighbor with minimum energy
    for (int y = 1; y < height; y++) {
        int current_x = seam[y - 1];
        int best_x = current_x;
        float best_energy = energy[y][current_x];
        
        // Check left neighbor
        if (current_x > 0 && energy[y][current_x - 1] < best_energy) {
            best_energy = energy[y][current_x - 1];
            best_x = current_x - 1;
        }
        
        // Check right neighbor
        if (current_x < width - 1 && energy[y][current_x + 1] < best_energy) {
            best_energy = energy[y][current_x + 1];
            best_x = current_x + 1;
        }
        
        seam[y] = best_x;
    }
    
    return seam;
}

std::vector<int> find_low_energy_seam_dyn(
    const std::vector<std::vector<float>>& energy, 
    int width, 
    int height
) {
    // Step 1: Create DP table to store minimum cumulative energy to reach each pixel
    std::vector<std::vector<float>> dp(height, std::vector<float>(width, std::numeric_limits<float>::max()));
    
    // Step 2: Initialize first row - cumulative energy equals pixel energy
    for (int x = 0; x < width; x++) {
        dp[0][x] = energy[0][x];
    }
    
    // Step 3: Fill DP table row by row using recurrence relation
    for (int y = 1; y < height; y++) {
        for (int x = 0; x < width; x++) {
            // Consider all possible previous positions that can reach current pixel (x, y)
            // Due to connectivity constraint, can only come from: (y-1, x-1), (y-1, x), (y-1, x+1)
            
            // Can come from directly above: (y-1, x)
            dp[y][x] = dp[y-1][x] + energy[y][x];
            
            // Can come from upper-left diagonal: (y-1, x-1)
            if (x > 0) {
                dp[y][x] = std::min(dp[y][x], dp[y-1][x-1] + energy[y][x]);
            }
            
            // Can come from upper-right diagonal: (y-1, x+1)  
            if (x < width - 1) {
                dp[y][x] = std::min(dp[y][x], dp[y-1][x+1] + energy[y][x]);
            }
        }
    }
    
    // Step 4: Find the ending position with minimum cumulative energy in bottom row
    int min_end_x = 0;
    float min_cumulative_energy = dp[height-1][0];
    for (int x = 1; x < width; x++) {
        if (dp[height-1][x] < min_cumulative_energy) {
            min_cumulative_energy = dp[height-1][x];
            min_end_x = x;
        }
    }
    
    // Step 5: Backtrack to reconstruct the optimal seam path
    std::vector<int> seam(height);
    seam[height-1] = min_end_x;  // Start from optimal ending position
    
    for (int y = height - 2; y >= 0; y--) {
        int current_x = seam[y + 1];
        int best_prev_x = current_x;
        float best_prev_energy = dp[y][current_x];
        
        // Check which previous position led to current optimal path
        // Left diagonal: (y, current_x - 1)
        if (current_x > 0 && dp[y][current_x - 1] < best_prev_energy) {
            best_prev_energy = dp[y][current_x - 1];
            best_prev_x = current_x - 1;
        }
        
        // Right diagonal: (y, current_x + 1)
        if (current_x < width - 1 && dp[y][current_x + 1] < best_prev_energy) {
            best_prev_energy = dp[y][current_x + 1];
            best_prev_x = current_x + 1;
        }
        
        seam[y] = best_prev_x;
    }
    
    return seam;
}

std::vector<int> find_low_energy_seam(
    const std::vector<std::vector<float>>& energy, 
    int width, 
    int height,
    Algorithm algorithm
) {
    switch (algorithm) {
        case Algorithm::GREEDY:
            return find_low_energy_seam_greedy(energy, width, height);
        case Algorithm::DYNAMIC:
            return find_low_energy_seam_dyn(energy, width, height);
        default:
            return find_low_energy_seam_greedy(energy, width, height);
    }
}

} // namespace SeamCarving