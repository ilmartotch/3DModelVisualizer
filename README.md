# BallOfWool - 3D Visualizer Engine

**BallOfWool** is a real-time 3D visualization and manipulation engine built with modern **C++20** and **OpenGL 4.5**. Designed as a sandbox environment, it allows for the loading, inspection, and manipulation of 3D assets with a focus on lighting and rendering control.

> **⚠️ Work In Progress**: This project is currently under active development. A major refactor is ongoing to transition towards a more robust architecture, aiming to better leverage modern graphics APIs, improve hardware resource optimization, and increase modularity.

## 🛠 Technology Stack & Implementation

The engine is built upon a strictly typed, modern C++ codebase, leveraging **vcpkg** in manifest mode to handle the entire dependency graph, ensuring reproducible builds across different environments. The build system is orchestrated by **CMake**, facilitating the definition of export targets and resource management.

*   **Core Graphics**: **OpenGL 4.5** (Core Profile) is used for the rendering backend, utilizing **GLAD** for loader generation and **GLFW** for windowing and context management.
*   **Math & Physics**: **GLM** handles all vector and matrix mathematics, strictly following GLSL conventions for seamless shader integration.
*   **Asset Management**: **Assimp** is integrated to handle complex model importation, supporting a wide variety of formats (FBX, OBJ, GLTF), while **stb_image** manages texture loading.
*   **User Interface**: The UI is built with **ImGui**, providing a docking-capable interface for property inspection, coupled with **ImGuizmo** to project 3D manipulation controls directly into the scene view.

## 🌟 Core Features

### Scene & Asset Management
Unlike standard viewers, BallOfWool offers distinct import strategies. Users can load complex scenes and choose to interpret them as a **Single Object** (for easier global manipulation) or **Separate Meshes** (preserving the hierarchy for individual part manipulation). The engine supports dynamic drag-and-drop for images, instantly converting them into textured planes within the 3D space. The environment can be toggled between an **Infinite Grid** for unbounded creativity and a **Finite Space** mode, which enforces logical boundaries and object limits, useful for performance testing or specific game-design constraints.

### Advanced Rendering Pipeline
The visual output is driven by a custom forward rendering pipeline that emphasizes depth and spatial perception:
*   **Hybrid Shadow System**: Combines standard **Directional Shadow Mapping** (via depth framebuffers) for self-shadowing of objects with **Planar Contact Shadows** projected onto the floor. This dual approach ensures objects feel grounded and not "floating," providing realistic visual feedback during manipulation.
*   **Picking System**: Pixel-perfect object selection is achieved via a dedicated framebuffer pass that renders object IDs to an off-screen texture, ensuring 100% accuracy regardless of mesh complexity or distance from the camera.

## ⚙️ Architecture & Execution Pipeline

The application structure is designed to decouple resource management from the rendering loop, ensuring that heavy I/O operations do not block the frame execution where possible.

### Initialization & Resource Loading
Upon launch, the `Window` subsystem initializes the OpenGL context, followed by the `ModelManager` which pre-loads geometric primitives and shaders. The system utilizes a **Flyweight pattern** for mesh data; multiple instances of the same object in the scene share the same underlying vertex data (VBOs/VAOs) to minimize VRAM usage.

### The Render Loop
The execution flow within `main.cpp` orchestrates distinct passes for every frame:

1.  **Shadow Pass**: The scene is rendered from the light's perspective into a high-resolution depth map. This pass uses a simplified shader (`DirShadowDepth`) that strips out fragment calculations to maximize performance.
2.  **Logic & Input**: The `SceneManager` updates object transformations based on user input or active Gizmo operations.
3.  **Picking Pass (On-Demand)**: When interaction occurs, a specific render pass draws the scene using unique color IDs to an off-screen buffer. The pixel under the mouse cursor is read back to identify the specific `SceneObject` selected.
4.  **Main Render Pass**: The scene is rendered to the backbuffer. Shaders consume the shadow map generated in step 1 to calculate lighting, applying PCF (Percentage-Closer Filtering) for soft shadow edges.
5.  **UI Overlay**: Finally, ImGui renders the interface hierarchy on top of the 3D scene before the buffer swap.

## 🚀 Build & Installation

Prerequisites: **Visual Studio 2022** (or a C++20 compatible compiler), **CMake**, and **Git**.

1.  **Clone the Repository**:
    ```bash
    git clone https://github.com/your-username/BallOfWool.git
    cd BallOfWool
    ```

2.  **Configure with CMake**:
    The project relies on `vcpkg.json` to automatically download and build dependencies.
    ```bash
    # Generate build files (Visual Studio will detect the vcpkg toolchain automatically)
    cmake -B out/build -S . -DCMAKE_TOOLCHAIN_FILE=[path/to/vcpkg]/scripts/buildsystems/vcpkg.cmake
    ```

3.  **Build**:
    Compile the project. This will also handle the post-build events that copy assets and shaders to the executable directory.
    ```bash
    cmake --build out/build --config Release
    ```

## 💬 Community & Feedback

The project is evolving, and feedback is highly appreciated. The **GitHub Issues** section is open and actively monitored for:
*   Bug reports and reproduction steps.
*   Feature requests for future iterations.
*   Suggestions regarding the architectural refactor or graphics optimization.

Feel free to open a discussion or a pull request if you wish to contribute to the codebase.
