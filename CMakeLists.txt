cmake_minimum_required(VERSION 3.28)
# Specifica il toolchain di vcpkg
set(CMAKE_TOOLCHAIN_FILE "${CMAKE_SOURCE_DIR}/vcpkg/scripts/buildsystems/vcpkg.cmake")

project(MyOpenGLProject LANGUAGES CXX)

# Imposta standard C++
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_VS_DEBUGGER_WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})

# src/CMakeLists.txt
add_executable(App
  src/main.cpp
 "src/Include/Window.h" "src/Source/Window.cpp" "src/Source/Shaders.cpp" "src/Include/Shaders.h")

# Trova pacchetti con vcpkg
find_package(glfw3 CONFIG REQUIRED)
find_package(glad CONFIG REQUIRED)
find_package(imgui CONFIG REQUIRED)
#find_package(stb CONFIG REQUIRED)
find_package(OpenGL REQUIRED)

target_link_libraries(App PRIVATE
  glfw
  glad::glad
  #imgui::imgui
  #stb::stb
  OpenGL::GL
)

set(SHADERS_DIR ${CMAKE_SOURCE_DIR}/Shaders)

# Copia automatica degli shader nella cartella dell'eseguibile dopo il build
add_custom_command(TARGET App POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${SHADERS_DIR}
        $<TARGET_FILE_DIR:App>/Shaders
)

# Include directories
target_include_directories(App PRIVATE
    ${CMAKE_SOURCE_DIR}/src/Include
)

target_include_directories(App PRIVATE
  ${CMAKE_SOURCE_DIR}/shaders
)