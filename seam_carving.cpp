#include "seam_carving.h"
#include <limits>
#include <algorithm>
#include <cmath>
#include <chrono>
#include <spdlog/spdlog.h>

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

std::vector<std::vector<float>> calculate_energy(
    const unsigned char* pixels, int width, int height, int channels) {
  
  std::vector<std::vector<float>> energy(height, std::vector<float>(width, 0.0f));
  
  // Sobel kernels
  int sobel_x[3][3] = {{-1, 0, 1}, {-2, 0, 2}, {-1, 0, 1}};
  int sobel_y[3][3] = {{-1, -2, -1}, {0, 0, 0}, {1, 2, 1}};
  
  for (int y = 1; y < height - 1; y++) {
    for (int x = 1; x < width - 1; x++) {
      float gx = 0, gy = 0;
      
      // Apply Sobel kernels with proper convolution
      for (int ky = -1; ky <= 1; ky++) {
        for (int kx = -1; kx <= 1; kx++) {
          int idx = ((y + ky) * width + (x + kx)) * channels;
          
          // Convert to grayscale
          float gray = 0.299f * pixels[idx] + 0.587f * pixels[idx + 1] + 0.114f * pixels[idx + 2];
          
          gx += gray * sobel_x[ky + 1][kx + 1];
          gy += gray * sobel_y[ky + 1][kx + 1];
        }
      }
      
      energy[y][x] = sqrt(gx * gx + gy * gy);
    }
  }
  
  return energy;
}

std::vector<unsigned char> remove_seam(
    const unsigned char* pixels,
    int width,
    int height,
    int channels,
    const std::vector<int>& seam
) {
    // Calculate new width and allocate memory for result
    int new_width = width - 1;
    std::vector<unsigned char> result(new_width * height * channels);
    
    // For each row, copy pixels excluding the seam pixel
    for (int y = 0; y < height; y++) {
        int seam_x = seam[y];
        int result_idx = 0;
        
        for (int x = 0; x < width; x++) {
            if (x != seam_x) {
                // Copy pixel to new image
                int src_idx = (y * width + x) * channels;
                int dst_idx = (y * new_width + result_idx) * channels;
                
                for (int c = 0; c < channels; c++) {
                    result[dst_idx + c] = pixels[src_idx + c];
                }
                result_idx++;
            }
        }
    }
    
    return result;
}

std::pair<std::vector<unsigned char>, int> reduce_width_iteratively(
    const unsigned char* pixels,
    int original_width,
    int height,
    int channels,
    int target_width,
    Algorithm algorithm
) {
    if (target_width >= original_width) {
        // No reduction needed, return copy of original
        std::vector<unsigned char> result(pixels, pixels + (original_width * height * channels));
        return std::make_pair(result, original_width);
    }
    
    if (target_width <= 0) {
        // Invalid target width
        std::vector<unsigned char> empty;
        return std::make_pair(empty, 0);
    }
    
    // Start with copy of original image
    std::vector<unsigned char> current_pixels(pixels, pixels + (original_width * height * channels));
    int current_width = original_width;
    
    int seams_to_remove = original_width - target_width;
    int seams_removed = 0;
      
    // Progress tracking variables
    int progress_update_interval = std::max(1, seams_to_remove / 10); // Update every 10% or at least every seam
    auto batch_start_time = std::chrono::high_resolution_clock::now();
      
    spdlog::info("Starting seam carving: removing {} seams from {}x{} image", 
                 seams_to_remove, original_width, height);
      
    // Iteratively remove seams until we reach target width
    while (current_width > target_width) {
        seams_removed++;
        
        if (seams_removed % progress_update_interval == 0 || seams_removed == seams_to_remove) {
            auto current_time = std::chrono::high_resolution_clock::now();
            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(current_time - batch_start_time);
            float avg_time_per_seam = static_cast<float>(elapsed.count()) / seams_removed;
            int estimated_remaining_ms = static_cast<int>((seams_to_remove - seams_removed) * avg_time_per_seam);
            
            spdlog::info("Progress: {}/{} seams removed ({}% complete) - Avg: {:.1f}ms/seam, ETA: {}s", 
                         seams_removed, seams_to_remove,
                         static_cast<int>((seams_removed * 100.0) / seams_to_remove),
                         avg_time_per_seam,
                         estimated_remaining_ms / 1000);
        }
        
        auto start_time = std::chrono::high_resolution_clock::now();
        
        // Calculate energy for current image state
        auto energy = calculate_energy(current_pixels.data(), current_width, height, channels);
        
        // Find optimal seam to remove
        auto seam = find_low_energy_seam(energy, current_width, height, algorithm);
        
        // Remove the seam from current image
        current_pixels = remove_seam(current_pixels.data(), current_width, height, channels, seam);
        current_width--;
        
        // Log detailed timing only for debug builds or when specifically enabled
        if (spdlog::get_level() <= spdlog::level::debug) {
            auto end_time = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
            if (duration.count() > 50) { // Only log slow iterations
                spdlog::debug("Slow iteration {} took {}ms (width: {})", seams_removed, duration.count(), current_width + 1);
            }
        }
    }
    
    spdlog::info("Seam carving completed: final image size {}x{}", current_width, height);
    
    return std::make_pair(current_pixels, current_width);
}

} // namespace SeamCarving
