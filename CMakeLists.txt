cmake_minimum_required(VERSION 3.28)
# Specifica il toolchain di vcpkg
set(CMAKE_TOOLCHAIN_FILE "${CMAKE_SOURCE_DIR}/vcpkg/scripts/buildsystems/vcpkg.cmake")

project(MyOpenGLProject LANGUAGES CXX)

# Imposta standard C++
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_VS_DEBUGGER_WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})

# src/CMakeLists.txt
add_executable(App
  src/main.cpp
 "src/Include/Window.h" "src/Source/Window.cpp" 
 "src/Source/Shaders.cpp" "src/Include/Shaders.h" 
 "src/Include/Model.h" "src/Source/Model.cpp" 
 "Assets/Include/PyramidModel.h" "Assets/Source/PyramidModel.cpp" 
 "Assets/Include/CubeModel.h" "Assets/Source/CubeModel.cpp" 
 "Assets/Include/SphereModel.h" "Assets/Source/SphereModel.cpp" 
 "src/Include/Grid.h" "src/Source/Grid.cpp" 
 "src/Include/ModelManager.h" "src/Source/ModelManager.cpp" 
 "src/Include/InstancedModelManager.h" "src/Source/InstancedModelManager.cpp" 
 "src/Source/PickingBuffer.cpp" "src/Include/SceneObject.h" 
 "src/Include/SceneManager.h" "src/Source/SceneManager.cpp"
 "src/Include/ModelLoader.h" "src/Source/ModelLoader.cpp"
 "src/Source/stb_image_impl.cpp"
 "src/Include/TextureManager.h" "src/Source/TextureManager.cpp"
 "Assets/Include/ImagePlaneModel.h" "Assets/Source/ImagePlaneModel.cpp"
)

# Trova pacchetti con vcpkg
find_package(glfw3 CONFIG REQUIRED)
find_package(glad CONFIG REQUIRED)
find_package(imgui CONFIG REQUIRED)
find_package(Stb REQUIRED)
find_package(OpenGL REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(imguizmo CONFIG REQUIRED)
find_package(assimp REQUIRED)

target_include_directories(App PRIVATE ${Stb_INCLUDE_DIR})

target_compile_definitions(App PRIVATE IMGUI_IMPL_OPENGL_GLAD)

target_link_libraries(App PRIVATE
  glfw
  glad::glad
  imgui::imgui
  OpenGL::GL
  glm::glm
  imguizmo::imguizmo
  assimp::assimp
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
  ${CMAKE_SOURCE_DIR}/Shaders
)