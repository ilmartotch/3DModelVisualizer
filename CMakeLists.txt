cmake_minimum_required(VERSION 3.28)
# Specifica il toolchain di vcpkg
set(CMAKE_TOOLCHAIN_FILE "${CMAKE_SOURCE_DIR}/vcpkg/scripts/buildsystems/vcpkg.cmake")

project(MyOpenGLProject LANGUAGES CXX)

# Imposta standard C++
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


# src/CMakeLists.txt
add_executable(App
  src/main.cpp
)

# Trova pacchetti con vcpkg
find_package(glfw3 CONFIG REQUIRED)
find_package(glad CONFIG REQUIRED)
find_package(imgui CONFIG REQUIRED) # se usi ImGui

target_link_libraries(App PRIVATE
  glfw
  glad::glad
  #imgui
)

target_include_directories(App PRIVATE
  ${CMAKE_SOURCE_DIR}/shaders
)