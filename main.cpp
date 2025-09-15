#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include <glad/glad.h>

#include <spdlog/spdlog.h>

#include <GLFW/glfw3.h> // Will drag system OpenGL headers
#include <fmt/format.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include "seam_carving.h"

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

// Helper function to create/update OpenGL texture
bool create_or_update_texture(GLuint& texture_id, const void* data, int width, int height, const std::string& description) {
  if (texture_id == 0) {
    glGenTextures(1, &texture_id);
    if (glGetError() != GL_NO_ERROR) {
      spdlog::error("Failed to generate texture for {}", description);
      return false;
    }
  }
  
  glBindTexture(GL_TEXTURE_2D, texture_id);
  if (glGetError() != GL_NO_ERROR) {
    spdlog::error("Failed to bind texture for {}", description);
    return false;
  }
  
  // Set pixel alignment
  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
  
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, 
               GL_UNSIGNED_BYTE, data);
  
  GLenum error = glGetError();
  if (error != GL_NO_ERROR) {
    spdlog::error("Failed to upload texture data for {}: OpenGL error {}", description, error);
    return false;
  }
  
  spdlog::info("Successfully created/updated OpenGL texture for {}: {}x{}", description, width, height);
  return true;
}

// Bilinear interpolation downscaling function
unsigned char* downscale_image_bilinear(const unsigned char* src_data, 
                                       int src_width, int src_height, int channels,
                                       int dst_width, int dst_height) {
  unsigned char* dst_data = new unsigned char[dst_width * dst_height * channels];
  
  float x_ratio = (float)src_width / dst_width;
  float y_ratio = (float)src_height / dst_height;
  
  for (int y = 0; y < dst_height; y++) {
    for (int x = 0; x < dst_width; x++) {
      float src_x = x * x_ratio;
      float src_y = y * y_ratio;
      
      int x1 = (int)src_x;
      int y1 = (int)src_y;
      int x2 = std::min(x1 + 1, src_width - 1);
      int y2 = std::min(y1 + 1, src_height - 1);
      
      float dx = src_x - x1;
      float dy = src_y - y1;
      
      for (int c = 0; c < channels; c++) {
        // Get the four surrounding pixels
        float p11 = src_data[(y1 * src_width + x1) * channels + c];
        float p12 = src_data[(y1 * src_width + x2) * channels + c];
        float p21 = src_data[(y2 * src_width + x1) * channels + c];
        float p22 = src_data[(y2 * src_width + x2) * channels + c];
        
        // Bilinear interpolation
        float interpolated = p11 * (1 - dx) * (1 - dy) +
                           p12 * dx * (1 - dy) +
                           p21 * (1 - dx) * dy +
                           p22 * dx * dy;
        
        dst_data[(y * dst_width + x) * channels + c] = (unsigned char)std::round(interpolated);
      }
    }
  }
  
  return dst_data;
}



int main(int, char **) {
  // Setup spdlog for optimized logging
  spdlog::set_level(spdlog::level::info);  // Show info and above (reduce verbose debug output)
  spdlog::set_pattern("[%H:%M:%S] [%l] %v");  // Show timestamp and level
  spdlog::info("Application starting...");

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
  
  // Global texture IDs for cleanup
  static GLuint original_texture_id = 0;
  static GLuint carved_texture_id = 0;
  static GLuint primitive_texture_id = 0;
  static unsigned char* image_data = nullptr;
  static unsigned char* primitive_resized_data = nullptr;

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
      static int img_w = 0, img_h = 0, img_channels = 0;
      static bool image_loaded = false;
      static float target_scale_perc = 100.0f;  // Scale percentage (10-100%)
      static SeamCarving::Algorithm selected_algorithm = SeamCarving::Algorithm::GREEDY;

      // 2. upload image to gpu
      if (!image_loaded) {
        unsigned char* temp_image_data = load_image(img_path, img_w, img_h, img_channels);
        if (temp_image_data) {
          spdlog::info("Image loaded successfully: {}x{}x{}", img_w, img_h, img_channels);
          
          // Store the image data for seam carving
          size_t data_size = img_w * img_h * img_channels;
          image_data = new unsigned char[data_size];
          std::memcpy(image_data, temp_image_data, data_size);
          
          // Create OpenGL texture using helper function
          if (create_or_update_texture(original_texture_id, temp_image_data, img_w, img_h, "original image")) {
            image_loaded = true;
            // Free the temp image data after uploading to GPU - stbi_image_free is for data from stbi_load
            stbi_image_free(temp_image_data);
          } else {
            stbi_image_free(temp_image_data);
            delete[] image_data;
            image_data = nullptr;
            return 1;
          }
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
      
      // Algorithm selection radio buttons
      ImGui::Text("Seam Finding Algorithm:");
      bool algo_changed = false;
      if (ImGui::RadioButton("Greedy (Fast)", selected_algorithm == SeamCarving::Algorithm::GREEDY)) {
        selected_algorithm = SeamCarving::Algorithm::GREEDY;
        algo_changed = true;
      }
      ImGui::SameLine();
      if (ImGui::RadioButton("Dynamic Programming (Optimal)", selected_algorithm == SeamCarving::Algorithm::DYNAMIC)) {
        selected_algorithm = SeamCarving::Algorithm::DYNAMIC;
        algo_changed = true;
      }
      
      if (algo_changed) {
        needs_recompute = true;
      }
      
      // Calculate target width based on slider value
      int target_width = (int)(img_w * target_scale_perc / 100.0f);
      
      // Static variables for processed image
      static std::vector<unsigned char> carved_image_data;
      static int carved_width = 0;
      static bool carved_image_valid = false;
        
      // Static variables for primitive resized image
      static bool primitive_image_valid = false;
      
      if (needs_recompute && image_loaded) {
        // Use iterative seam removal to reduce image width
        const char* algo_name = (selected_algorithm == SeamCarving::Algorithm::GREEDY) ? "Greedy" : "Dynamic Programming";
        spdlog::info("Starting iterative seam carving: {}x{} -> {}x{} using {} algorithm", 
                     img_w, img_h, target_width, img_h, algo_name);
        
        // Perform iterative seam removal
        auto result = SeamCarving::reduce_width_iteratively(
            image_data, img_w, img_h, img_channels, target_width, selected_algorithm);
        auto result_pixels = result.first;
        auto final_width = result.second;
        
        spdlog::info("Seam carving completed: final size {}x{}", final_width, img_h);
        
        // Store the result
        carved_image_data = std::move(result_pixels);
        carved_width = final_width;
        
        // Create/update OpenGL texture for carved image
        carved_image_valid = create_or_update_texture(carved_texture_id, carved_image_data.data(), carved_width, img_h, "carved image");
      
        // Create primitive resized image using bilinear interpolation
        spdlog::info("Creating primitive resized image using bilinear interpolation: {}x{} -> {}x{}", 
                     img_w, img_h, target_width, img_h);
        
        // Clean up previous primitive resized data
        if (primitive_resized_data) {
          delete[] primitive_resized_data;
        }
        
        // Create resized pixel array using bilinear interpolation (horizontal scaling only)
        primitive_resized_data = downscale_image_bilinear(
            image_data, img_w, img_h, img_channels, target_width, img_h);
        
        // Create/update OpenGL texture for primitive resized image
        primitive_image_valid = create_or_update_texture(primitive_texture_id, primitive_resized_data, target_width, img_h, "primitive resized image");
      }

      ImGui::Text("Processed (Seam Carved)");
      if (carved_image_valid && carved_texture_id) {
        ImGui::Image((ImTextureID)(intptr_t)carved_texture_id, ImVec2(carved_width, img_h));
        ImGui::Text("Carved image size: %dx%d (removed %d seams)", carved_width, img_h, img_w - carved_width);
      } else {
        ImGui::Text("Move the slider to see seam carved result");
      }

      ImGui::Text("Primitive Resized");
      if (primitive_image_valid && primitive_texture_id) {
        ImGui::Image((ImTextureID)(intptr_t)primitive_texture_id, ImVec2(target_width, img_h));
        ImGui::Text("Primitive resized image size: %dx%d (bilinear interpolation)", target_width, img_h);
      } else {
        ImGui::Text("Move the slider to see primitive resized result");
      }

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

  // Cleanup OpenGL textures
  GLuint textures_to_delete[] = {original_texture_id, carved_texture_id, primitive_texture_id};
  for (GLuint texture : textures_to_delete) {
    if (texture != 0) {
      glDeleteTextures(1, &texture);
    }
  }
  
  // Cleanup image data
  if (image_data) {
    delete[] image_data;
  }
  if (primitive_resized_data) {
    delete[] primitive_resized_data;
  }

  // Cleanup ImGui and GLFW
  ImGui_ImplOpenGL3_Shutdown();
  ImGui_ImplGlfw_Shutdown();
  ImGui::DestroyContext();

  glfwDestroyWindow(window);
  glfwTerminate();

  return 0;
}
