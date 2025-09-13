#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include <glad/glad.h>

#include <spdlog/spdlog.h>

#include <GLFW/glfw3.h> // Will drag system OpenGL headers
#include <fmt/format.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <string>
#include <vector>
#include <cmath>

static void glfw_error_callback(int error, const char *description) {
  fprintf(stderr, "Glfw Error %d: %s\n", error, description);
}

// load image
unsigned char *load_image(const std::string &path, int &width, int &height,
                          int &channels) {
  unsigned char *pixels =
      stbi_load(path.c_str(), &width, &height, &channels, STBI_rgb);
  if (!pixels) {
    spdlog::error("Failed to load image: {}", path.c_str());
  }
  return pixels;
}

// Calculate energy map using gradient approach (simple Sobel-like operator)
std::vector<std::vector<float>> calculate_energy(const unsigned char* pixels, int width, int height, int channels) {
  std::vector<std::vector<float>> energy(height, std::vector<float>(width, 0.0f));
  
  for (int y = 1; y < height - 1; y++) {
    for (int x = 1; x < width - 1; x++) {
      // Get current pixel index
      int idx = (y * width + x) * channels;
      
      // Calculate horizontal gradient (Gx)
      int left_idx = (y * width + (x - 1)) * channels;
      int right_idx = (y * width + (x + 1)) * channels;
      
      float gx_r = pixels[right_idx] - pixels[left_idx];
      float gx_g = pixels[right_idx + 1] - pixels[left_idx + 1];
      float gx_b = pixels[right_idx + 2] - pixels[left_idx + 2];
      
      // Calculate vertical gradient (Gy)
      int top_idx = ((y - 1) * width + x) * channels;
      int bottom_idx = ((y + 1) * width + x) * channels;
      
      float gy_r = pixels[bottom_idx] - pixels[top_idx];
      float gy_g = pixels[bottom_idx + 1] - pixels[top_idx + 1];
      float gy_b = pixels[bottom_idx + 2] - pixels[top_idx + 2];
      
      // Calculate energy as magnitude of gradient
      float gx_mag = sqrt(gx_r * gx_r + gx_g * gx_g + gx_b * gx_b);
      float gy_mag = sqrt(gy_r * gy_r + gy_g * gy_g + gy_b * gy_b);
      
      energy[y][x] = gx_mag + gy_mag;
    }
  }
  
  return energy;
}

// Find low energy vertical seams using greedy approach
std::vector<int> find_low_energy_seam(const std::vector<std::vector<float>>& energy, int width, int height) {
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

int main(int, char **) {
  // Setup window
  glfwSetErrorCallback(glfw_error_callback);
  if (!glfwInit())
    return 1;

  // Decide GL+GLSL versions
#if defined(IMGUI_IMPL_OPENGL_ES2)
  // GL ES 2.0 + GLSL 100
  const char *glsl_version = "#version 100";
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
  glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
#elif defined(__APPLE__)
  // GL 3.2 + GLSL 150
  const char *glsl_version = "#version 150";
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE); // 3.2+ only
  glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // Required on Mac
#else
  // GL 3.0 + GLSL 130
  const char *glsl_version = "#version 130";
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
  // glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);  // 3.2+
  // only glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // 3.0+ only
#endif

  // Create window with graphics context
  GLFWwindow *window = glfwCreateWindow(
      1280, 720, "Dear ImGui GLFW+OpenGL3 example", NULL, NULL);
  if (window == NULL)
    return 1;
  glfwMakeContextCurrent(window);
  glfwSwapInterval(1); // Enable vsync

  // Setup Dear ImGui context
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGuiIO &io = ImGui::GetIO();
  (void)io;
  io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
  io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;

  // Setup Dear ImGui style
  ImGui::StyleColorsDark();
  // ImGui::StyleColorsLight();

  // When viewports are enabled we tweak WindowRounding/WindowBg so platform
  // windows can look identical to regular ones.
  ImGuiStyle &style = ImGui::GetStyle();
  if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
    style.WindowRounding = 0.0f;
    style.Colors[ImGuiCol_WindowBg].w = 1.0f;
  }

  // Setup Platform/Renderer backends
  if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
    spdlog::error("Failed to initialize GLAD");
    return -1;
  }
  ImGui_ImplGlfw_InitForOpenGL(window, true);
  ImGui_ImplOpenGL3_Init(glsl_version);

  // Our state
  static bool show_demo_window = false;

  ImVec4 clear_color = ImVec4(0.168f, 0.394f, 0.534f, 1.00f);

  // Main loop
  while (!glfwWindowShouldClose(window)) {
    glfwPollEvents();

    // Start the Dear ImGui frame
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    ImGuiWindowFlags window_flags =
        ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_DockNodeHost;
    ImGui::DockSpaceOverViewport(0, nullptr,
                                 ImGuiDockNodeFlags_PassthruCentralNode);

    // 1. Show the big demo window (Most of the sample code is in
    // ImGui::ShowDemoWindow()! You can browse its code to learn more about Dear
    // ImGui!).
    if (show_demo_window) {
      ImGui::ShowDemoWindow(&show_demo_window);
    }

    // 2. Simple window that we create ourselves. We use a Begin/End pair
    // to create a named window.
    {
      ImGui::Begin("Settings");
      ImGui::Text("Configure the App below."); // Display some text (you can use
                                               // a format strings too)
      ImGui::Checkbox(
          "Demo Window",
          &show_demo_window); // Edit bools storing our window open/close state

      ImGui::Text("Application average %.3f ms/frame (%.1f FPS)",
                  1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
      ImGui::End();
    }

    // 3. Show image window
    {
      ImGui::Begin("Image Window");
      // ----- START HERE -----
      const std::string img_path = ASSET_PATH "/schmetterling_mid.jpg";

      // 1. load image
      static unsigned char* image_data = nullptr;
      static int img_w = 0, img_h = 0, img_channels = 0;
      static GLuint original_texture_id = 0;
      static bool image_loaded = false;
      static float target_scale_perc = 100.0f;  // Scale percentage (10-100%)

      // 2. upload image to gpu
      if (!image_loaded) {
        image_data = load_image(img_path, img_w, img_h, img_channels);
        if (image_data) {
          // Create OpenGL texture
          glGenTextures(1, &original_texture_id);
          glBindTexture(GL_TEXTURE_2D, original_texture_id);
          glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
          glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
          glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
          glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
          glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, img_w, img_h, 0, GL_RGB, GL_UNSIGNED_BYTE, image_data);
            
          spdlog::info("Image loaded successfully: {}x{}x{}", img_w, img_h, img_channels);
          image_loaded = true;
        } else {
          spdlog::error("Failed to load image: {}", img_path);
          return 1;
        }
      }

      // 3. display image
      // (https://github.com/ocornut/imgui/wiki/Image-Loading-and-Displaying-Examples)
      ImGui::Text("Original");
      if (image_loaded && original_texture_id) {
        ImGui::Image((ImTextureID)(intptr_t)original_texture_id, ImVec2(img_w, img_h));
      } else {
        ImGui::Text("Failed to load image");
      }

      // 4. add a simple imgui slider here to scale image from 0 to 100%
      bool needs_recompute = false;
      if (ImGui::SliderFloat("Scale Image By", &target_scale_perc, 10.0f, 100.0f,"%.0f%%")) {
        needs_recompute = true;
      }
      
      // Calculate target width based on slider value
      int target_width = (int)(img_w * target_scale_perc / 100.0f);
      if (needs_recompute && image_loaded) {
        // 5. Compute image energy
        spdlog::info("Computing energy map for {}x{} image, target width: {}", img_w, img_h, target_width);
        auto orig_energy = calculate_energy(image_data, img_w, img_h, img_channels);
        spdlog::info("Energy map computed successfully");
        
        // 6. find low energy seams
        spdlog::info("Finding low energy seam for removal");
        auto approx_seam = find_low_energy_seam(orig_energy, img_w, img_h);
        spdlog::info("Found seam path from top to bottom");

        // 7. now remove seems to reduce image size

        // 8. Primitive rescaling using for example bilinear interpolation
        // (horizontal only)
      }

      ImGui::Text("Processed (Seam Carved)");
      // ImGui::Image((ImTextureID)(intptr_t)image_processed_tex_id,
      //              ImVec2(working_width, img_h));

      ImGui::Text("Primitive Resized");
      // ImGui::Image((ImTextureID)(intptr_t)primitive_tex_id,
      //              ImVec2(target_scale, img_h));

      ImGui::End();
    }

    // Rendering
    ImGui::Render();
    int display_w, display_h;
    glfwGetFramebufferSize(window, &display_w, &display_h);
    glViewport(0, 0, display_w, display_h);
    glClearColor(clear_color.x * clear_color.w, clear_color.y * clear_color.w,
                 clear_color.z * clear_color.w, clear_color.w);
    glClear(GL_COLOR_BUFFER_BIT);
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    // Update and Render additional Platform Windows
    // (Platform functions may change the current OpenGL context, so we
    // save/restore it to make it easier to paste this code elsewhere.
    //  For this specific demo app we could also call
    //  glfwMakeContextCurrent(window) directly)
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
      GLFWwindow *backup_current_context = glfwGetCurrentContext();
      ImGui::UpdatePlatformWindows();
      ImGui::RenderPlatformWindowsDefault();
      glfwMakeContextCurrent(backup_current_context);
    }

    glfwSwapBuffers(window);
  }

  // Cleanup
  ImGui_ImplOpenGL3_Shutdown();
  ImGui_ImplGlfw_Shutdown();
  ImGui::DestroyContext();

  glfwDestroyWindow(window);
  glfwTerminate();

  return 0;
}
