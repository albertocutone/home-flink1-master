#include "seam_carving.h"
#include "gpu_energy.h"
#include <algorithm>
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

std::vector<int> find_low_energy_seam_dyn(const std::vector<std::vector<float>>& energy, int width, int height) {
  // Rolling array optimization: use only two rows instead of full DP table
  std::vector<float> prev_row(width);
  std::vector<float> curr_row(width);
  std::vector<std::vector<int>> backtrack(height, std::vector<int>(width, 0));
  
  // Initialize first row
  for (int x = 0; x < width; ++x) {
    prev_row[x] = energy[0][x];
  }
  
  // DP: accumulate minimum energy for each pixel using rolling arrays
  for (int y = 1; y < height; ++y) {
    for (int x = 0; x < width; ++x) {
      float min_energy = prev_row[x];
      int min_x = x;
      if (x > 0 && prev_row[x-1] < min_energy) {
        min_energy = prev_row[x-1];
        min_x = x-1;
      }
      if (x < width-1 && prev_row[x+1] < min_energy) {
        min_energy = prev_row[x+1];
        min_x = x+1;
      }
      curr_row[x] = energy[y][x] + min_energy;
      backtrack[y][x] = min_x;
    }
    // Swap rows for next iteration
    prev_row.swap(curr_row);
  }
  
  // Find end of minimum seam (result is now in prev_row after final swap)
  int min_end_x = 0;
  float min_total_energy = prev_row[0];
  for (int x = 1; x < width; ++x) {
    if (prev_row[x] < min_total_energy) {
      min_total_energy = prev_row[x];
      min_end_x = x;
    }
  }
  
  // Backtrack to get seam
  std::vector<int> seam(height);
  seam[height-1] = min_end_x;
  for (int y = height-1; y > 0; --y) {
    seam[y-1] = backtrack[y][seam[y]];
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
    const unsigned char* pixels, 
    int width, 
    int height, 
    int channels
) {
    auto start_time = std::chrono::high_resolution_clock::now();
    
    // Use GPU energy calculation
    auto gpu_result = GPUEnergy::calculate_energy_gpu(pixels, width, height, channels);
    
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);
    spdlog::info("GPU energy calculation completed in {:.2f}ms", duration.count() / 1000.0);
    
    return gpu_result;
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
