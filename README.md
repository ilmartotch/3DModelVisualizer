# 3D Model Visualizer

https://github.com/user-attachments/assets/6c258177-04b9-42ed-a901-da15a3ff18db

![Cattura](https://github.com/user-attachments/assets/fa6c37d9-c6c3-4cd2-83d3-7d40cc79a2bb)
![Cattura1](https://github.com/user-attachments/assets/8c6397b6-9941-4111-8030-d05903934934)


3D Model Visualizer is a real-time 3D visualization engine built with C++20 and Modern OpenGL (4.5). Designed as a sandbox environment, it allows for the loading, inspection, 
and manipulation of 3D assets with a focus on lighting control and rendering pipeline understanding. This project serves as a portfolio piece to demonstrate low-level graphics programming skills. 
While features like the picking system and shadow mapping are fully functional, the codebase is currently undergoing a refactor to transition from the current monolithic structure towards a more modular architecture.

## Technology Stack & Key Features

The engine is built upon a strictly typed C++ codebase using vcpkg for dependency management and CMake for the build system.

* **Core Architecture**
    Built with C++20 and OpenGL 4.5 (Core Profile). Uses GLFW for window management and GLAD for loader generation.

* **Hybrid Shadow System**
    Combines standard Directional Shadow Mapping (PCF filtered) for object self-shadowing with Planar Contact Shadows to strictly ground objects on the floor surface.

* **Pixel-Perfect Picking**
    Object selection is achieved via a dedicated framebuffer pass that renders object IDs to an off-screen texture, ensuring 100% accuracy regardless of mesh complexity.

* **Smart Asset Management**
    Uses Assimp to support various formats (FBX, OBJ, GLTF) with options to import scenes as a Single Object or Separate Meshes. Implements the Flyweight pattern to share vertex data across instances and optimize VRAM.

* **Interactive UI**
    Features a custom interface built with ImGui (docking enabled) and ImGuizmo for direct 3D manipulation. Includes a custom in-app log console to debug import errors directly within the viewport.

* **Math & Physics**
    Utilizes GLM for all vector and matrix mathematics, following GLSL conventions.

* **Environment Tools**
    Supports drag-and-drop for image textures (creating instant planes) and toggleable infinite/finite grid systems.

## Build & Installation

Prerequisites: **Visual Studio 2022** (or a C++20 compatible compiler), **CMake**, and **Git**.

### 1. Clone the repository
Per scaricare il progetto insieme a tutte le dipendenze del sottomodulo vcpkg, utilizza il comando `--recursive`

```bash
git clone --recursive [https://github.com/ilmartotch/3DModelVisualizer.git](https://github.com/ilmartotch/3DModelVisualizer.git)
cd 3DModelVisualizer
```

### 2. Build the project

Once the toolchain path is correct, generate the build files and compile. CMake will automatically handle the dependencies defined in vcpkg.json.
  ```bash
  # Generate build files
  cmake -B out .

  # Compile
  cmake --build out --config Release
  ```

The build process includes post-build commands that automatically copy shaders and necessary assets to the executable directory to ensure the application runs correctly.

### Feedback
The project is evolving, and feedback regarding the architectural refactor, graphics optimization, or the shadow implementation is highly appreciated. Feel free to open an Issue or a Pull Request for suggestions and bug reports.
