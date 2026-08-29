#pragma once

// Enable native platform extensions for GLFW
#define GLFW_EXPOSE_NATIVE_WAYLAND
#define GLFW_EXPOSE_NATIVE_X11
#include <GLFW/glfw3.h>

#include <webgpu/webgpu.h>
#ifdef WEBGPU_BACKEND_WGPU
#  include <webgpu/wgpu.h>
#endif // WEBGPU_BACKEND_WGPU

#include <glfw3webgpu.h>

#ifdef __EMSCRIPTEN__
#  include <emscripten.h>
#endif // __EMSCRIPTEN__

#include <iostream>
#include <cassert>
#include <vector>

#include "shader.h"

struct App {

	bool init();
	void processInputs();
	void renderPass(WGPUCommandEncoder encoder, WGPUTextureView targetView);

	void loop();
	void cleanup();
	bool isRunning();

	GLFWwindow *window;

	WGPUDevice device;
	WGPUQueue queue;
	WGPUSurface surface;

	WGPURenderPipeline pipeline;
	WGPUBuffer vertexBuffer;

	private:
	uint32_t screenWidth = 1024;
	uint32_t screenHeight = 768;
};
