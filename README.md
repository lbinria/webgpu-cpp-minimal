# webgpu-template

A starter template for building native WebGPU applications in C++.

![triangle](./triangle.png)

# Features

 - GLFW-based window creation
 - Simple, standard WebGPU render pipeline
 - Renders a basic triangle
 - GitHub Actions workflow that:
    - Builds binaries for Windows, Linux, and macOS on every push
    - Packages and publishes build artifacts when a tag is created



# Prerequisites

## Linux (Debian / Ubuntu)

Eventually install if missing:

`sudo apt-get install -y build-essential cmake libxkbcommon-dev libxinerama-dev libxcursor-dev libxi-dev`

# Build & Run

`cmake -B build && cmake --build build --parallel && ./build/webgpu_cpp_minimal`