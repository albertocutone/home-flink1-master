cmake_minimum_required(VERSION 3.27)

## vcpkg
include(FetchContent)
include(cmake/vcpkg.cmake)

## project
project (Flink-Home DESCRIPTION "Flink-Home" LANGUAGES CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON) # for clangd

## Find dependencies
# libraries list
set(libraries)

find_package(glfw3 REQUIRED)
set(libraries ${libraries} glfw)
find_package(OpenGL REQUIRED)
find_package(imgui CONFIG REQUIRED)
set(libraries ${libraries} imgui::imgui)
find_package(glad CONFIG REQUIRED)
set(libraries ${libraries} glad::glad)
find_package(spdlog CONFIG REQUIRED)
set(libraries ${libraries} spdlog::spdlog)
find_package(fmt CONFIG REQUIRED)
set(libraries ${libraries} fmt::fmt)

## Create main executable
add_executable(Flink-Home ${CMAKE_SOURCE_DIR}/main.cpp)

target_include_directories(
	Flink-Home
  	PRIVATE 
)

target_link_libraries(Flink-Home PRIVATE ${libraries})

# encode asset path
target_compile_definitions(Flink-Home PRIVATE ASSET_PATH="${CMAKE_SOURCE_DIR}/assets")
target_compile_definitions(Flink-Home PRIVATE FMT_HEADER_ONLY)
