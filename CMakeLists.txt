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
  # Main
  src/main.cpp
  
  # Core
  "src/Include/Window.h" "src/Source/Window.cpp"
  "src/Include/Shaders.h" "src/Source/Shaders.cpp"
  "src/Include/Model.h" "src/Source/Model.cpp"
  "src/Include/Grid.h" "src/Source/Grid.cpp"

  # Shadows
  "src/Include/ShadowSystem.h" "src/Source/ShadowSystem.cpp"
  "src/Include/ShadowFloor.h" "src/Source/ShadowFloor.cpp"
  
  # Managers
  "src/Include/ModelManager.h" "src/Source/ModelManager.cpp"
  "src/Include/InstancedModelManager.h" "src/Source/InstancedModelManager.cpp"
  "src/Include/SceneManager.h" "src/Source/SceneManager.cpp"
  "src/Include/ModelLoader.h" "src/Source/ModelLoader.cpp"
  "src/Include/TextureManager.h" "src/Source/TextureManager.cpp"
  
  # Scene Objects
  "src/Include/SceneObject.h" "src/Source/PickingBuffer.cpp"
  
  # Assets - Built-in Models
  "Assets/Include/PyramidModel.h" "Assets/Source/PyramidModel.cpp"
  "Assets/Include/CubeModel.h" "Assets/Source/CubeModel.cpp"
  "Assets/Include/SphereModel.h" "Assets/Source/SphereModel.cpp"
  "Assets/Include/ImagePlaneModel.h" "Assets/Source/ImagePlaneModel.cpp"
  
  # Export
  "src/Include/AssimpSceneExporter.h" "src/Source/AssimpSceneExporter.cpp"
  
  # Utility
  "src/Source/stb_image_impl.cpp"
 "src/Source/ShadowFloor.cpp"
 "src/Include/Logger.h" "src/Source/Logger.cpp"
 "src/Include/ConsoleWindow.h" "src/Source/ConsoleWindow.cpp"
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
find_package(nlohmann_json CONFIG REQUIRED)

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
  nlohmann_json::nlohmann_json
)

set(SHADERS_DIR ${CMAKE_SOURCE_DIR}/Shaders)
set(ASSETS_IMAGES_DIR ${CMAKE_SOURCE_DIR}/Assets/Images)

# Copia automatica degli shader nella cartella dell'eseguibile dopo il build
add_custom_command(TARGET App POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${SHADERS_DIR}
        $<TARGET_FILE_DIR:App>/Shaders
)

# Copia automatica delle immagini (logo/icona) nella cartella dell'eseguibile
add_custom_command(TARGET App POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory
        $<TARGET_FILE_DIR:App>/Assets/Images
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${ASSETS_IMAGES_DIR}/logo.png
        $<TARGET_FILE_DIR:App>/Assets/Images/logo.png
    COMMENT "Copying logo resources to build directory"
)

# Include directories
target_include_directories(App PRIVATE
    ${CMAKE_SOURCE_DIR}/src/Include
)

target_include_directories(App PRIVATE
  ${CMAKE_SOURCE_DIR}/Shaders
)

target_include_directories(App PRIVATE
  ${CMAKE_SOURCE_DIR}/Assets/Include
)

# Imposta l'icona dell'applicazione su Windows
if(WIN32)
    set(APP_ICON_PATH "${CMAKE_SOURCE_DIR}/Assets/Images/app_icon.ico")
    if(EXISTS ${APP_ICON_PATH})
        # Crea file RC per l'icona
        configure_file(
            "${CMAKE_SOURCE_DIR}/app_icon.rc.in"
            "${CMAKE_BINARY_DIR}/app_icon.rc"
            @ONLY
        )
        target_sources(App PRIVATE "${CMAKE_BINARY_DIR}/app_icon.rc")
    endif()
endif()