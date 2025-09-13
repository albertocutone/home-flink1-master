# Seam Carving Image Resizing Assignment

## Overview

This home assignment involves implementing seam carving, a content-aware image resizing technique, to intelligently reduce the width of an image while preserving important features. You'll work within a provided C++ project that uses Dear ImGui for the UI, GLFW for window management, and OpenGL for rendering. The base code already sets up a simple GUI application with windows for displaying images.

Your goal is to:
- Load and display an original image.
- Implement seam carving to create a resized (width-reduced) version of the image.
- Implement a primitive resizing method (e.g., bilinear interpolation) for comparison.
- Add a UI slider to control the resizing scale (10% to 100% of original width).
- Display all three versions side-by-side: Original, Seam Carved (Processed), and Primitive Resized.

The project includes a sample image (`schmetterling_mid.jpg`) in the `assets` directory (referenced via `ASSET_PATH` macro). Focus on horizontal resizing only (reducing width while keeping height constant).

Check out how it could look like: 
[▶️ Watch the demo video](https://drive.google.com/file/d/11-djbHk8BaetzwKaqovz8AmSLBp9iEu2/view?usp=drive_link)

**Important Notes:**
- Seam carving involves computing an energy map of the image, finding the lowest-energy seams (paths of pixels), and removing them iteratively to shrink the image.
- For the primitive method, use a simple scaling approach like bilinear interpolation (you can implement it manually or use a library if available in the environment).
- The resizing should be dynamic: recompute only when the slider value changes to avoid unnecessary processing.

## Prerequisites

- A code editor that supports CMake projects (e.g., Visual Studio Code, CLion, or Visual Studio).
- CMake (version 3.20 or higher) installed on your system.
- A C++ compiler (e.g., GCC, Clang, or MSVC) that supports C++11 or later.
- Git (to clone the project repository)

No additional installations are required for dependencies—the project uses CMake's FetchContent to download vcpkg, which will automatically handle package installation (e.g., ImGui, GLFW, GLAD, stb_image, spdlog, fmt).

## Setup Instructions

1. **Open the Project:**
   - Clone or download the project repository.
   - Open the project root directory in your editor (e.g., in VSCode, use "Open Folder").
   - The `CMakeLists.txt` file defines the build configuration.

2. **Build the Project:**
   - CMake will automatically download and configure vcpkg via FetchContent.
   - In your editor:
     - If using VSCode with the CMake extension: Select "Configure" and then "Build" from the CMake toolbar.
     - If using command line: Create a `build` directory, run `cmake ..` inside it, then `cmake --build .`.
   - The executable will be generated in the build directory (e.g., `seam_carving_app` or similar).

3. **Run the Application:**
   - Execute the built binary.
   - A window titled "Flink-Home" will appear with docking enabled.
   - You'll see a "Settings" window and an "Image Window" with placeholders for the images.

The base code is in `main.cpp`, with the implementation starting point marked as `// ----- START HERE -----` inside the "Image Window" block.

## Assignment Tasks (Step-by-Step)

Implement the following in the "Image Window" section of the main loop. Build and test incrementally to ensure each step works.

1. **Load the Image:**
   - Use the provided `load_image` function (which wraps `stbi_load` from stb_image) to load the image from `const std::string img_path = ASSET_PATH "/schmetterling_mid.jpg";`.
   - Store the pixel data, width, height, and channels.
   - Handle errors if loading fails (already partially implemented via spdlog).

2. **Upload Image to GPU and Display the Original:**
   - Generate an OpenGL texture ID using `glGenTextures`.
   - Bind the texture with `glBindTexture(GL_TEXTURE_2D, texture_id)`.
   - Set texture parameters (e.g., `glTexParameteri` for filtering and wrapping).
   - Upload pixels with `glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, pixels)`.
   - Display it using `ImGui::Image((ImTextureID)(intptr_t)texture_id, ImVec2(width, height));` under the "Original" text.

3. **Add a Slider for Scale Control:**
   - Declare a static float variable, e.g., `static float target_scale_perc = 100.0f;`.
   - Use `ImGui::SliderFloat("Scale Image By", &target_scale_perc, 10.0f, 100.0f, "%.0f%%");`.
   - Compute the target width: `int target_width = static_cast<int>(original_width * (target_scale_perc / 100.0f));`.
   - Set a flag `bool needs_recompute = false;` and update it to `true` if the slider changes (use `ImGui::SliderFloat`'s return value).

4. **Implement Seam Carving (Processed Image):**
   - If `needs_recompute` is true:
     - Compute the energy map: For each pixel, calculate energy based on gradient (e.g., Sobel operator: difference in intensity between neighboring pixels horizontally and vertically).
     - Find low-energy seams: Use dynamic programming (start simple!! E.g. greedy algorithm) to find the vertical seam with the lowest cumulative energy.
     - Remove seams iteratively: Reduce the image width by removing one seam at a time until reaching the target width. Update the pixel array accordingly.
   - Upload the processed pixels to a new GPU texture.
   - Display it under "Processed (Seam Carved)" with the adjusted width: `ImGui::Image((ImTextureID)(intptr_t)processed_texture_id, ImVec2(target_width, height));`.

5. **Implement Primitive Resizing (for Comparison):**
   - If `needs_recompute` is true:
     - Create a resized pixel array using bilinear interpolation (horizontal scaling only).
     - For each pixel in the target image, interpolate from the original based on the scale factor.
   - Upload to another GPU texture.
   - Display it under "Primitive Resized" with the same target dimensions.

6. **Optimization and Cleanup:**
   - Free image data with `stbi_image_free` after uploading to GPU.
   - Delete textures on shutdown (add to cleanup section).
   - Ensure recomputation only happens when necessary to keep the app responsive.
   - Test with different scale percentages—aim for smooth performance even at low scales (e.g., 50%).

## Resources

- **Seam Carving Reference:** https://en.wikipedia.org/wiki/Seam_carving
- **ImGui Image Display Examples:** Check the ImGui wiki: https://github.com/ocornut/imgui/wiki/Image-Loading-and-Displaying-Examples.
- **Libraries Available:** Use stb_image for loading, spdlog for logging, fmt for formatting. No external installs needed.
- **Debugging:** Use spdlog::info/error for logs. The app supports ImGui's demo window (toggle via checkbox) for UI reference.

## Evaluation Criteria

- By far the most important is creativity and fun. If you struggle to implement things just implement something else. :) 
- Correct implementation of seam carving (preserves content better than primitive scaling).
- Functional UI slider and dynamic resizing.
- Clean, readable code with comments.
- Error handling (e.g., for invalid scales or loading failures).
- Performance: Should handle the sample image efficiently.


If you encounter issues with setup or have questions, reach out to the assignment coordinator. Good luck!