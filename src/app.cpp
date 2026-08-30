#include "app.h"
#include "helpers.h"

WGPURenderPipeline createPipeline(WGPUDevice device, WGPUTextureFormat surfaceFormat) {

	// Load shader module
	auto shaderModule = Shader::createShaderModule(device, "shaders/triangle.wgsl");

	// Create Pipeline Layout & Render Pipeline
    WGPUPipelineLayoutDescriptor layoutDesc = { .bindGroupLayoutCount = 0, .bindGroupLayouts = NULL };
    WGPUPipelineLayout pipelineLayout = wgpuDeviceCreatePipelineLayout(device, &layoutDesc);

	WGPUColorTargetState colorTarget = {
        .format = surfaceFormat,
        .blend = NULL,
        .writeMask = WGPUColorWriteMask_All
    };
	
    WGPUFragmentState fragmentState = {
        .module = shaderModule,
        .entryPoint = "fs_main",
        .targetCount = 1,
        .targets = &colorTarget
    };

	WGPURenderPipelineDescriptor pipelineDesc = {
		.layout = pipelineLayout,
		.vertex = {
			.module = shaderModule,
			.entryPoint = "vs_main",
			.bufferCount = 0,
			.buffers = NULL
		},
		.primitive = {
			.topology = WGPUPrimitiveTopology_TriangleList,
			.stripIndexFormat = WGPUIndexFormat_Undefined,
			.frontFace = WGPUFrontFace_CCW,
			.cullMode = WGPUCullMode_None
		},
		.depthStencil = NULL,
		.multisample = { .count = 1, .mask = ~0u, .alphaToCoverageEnabled = false },
		.fragment = &fragmentState,
	};

	WGPURenderPipeline pipeline = wgpuDeviceCreateRenderPipeline(device, &pipelineDesc);

	wgpuShaderModuleRelease(shaderModule);

	return pipeline;
}

bool App::init() {

	std::cout << "hello webgpu !" << std::endl;

	// We create a descriptor
	WGPUInstanceDescriptor desc = {};
	desc.nextInChain = nullptr;

	// We create the instance using this descriptor
	auto instance = wgpuCreateInstance(&desc);

	// We can check whether there is actually an instance created
	if (!instance) {
		std::cerr << "Could not initialize WebGPU!" << std::endl;
		return false;
	}

	// Display the object (WGPUInstance is a simple pointer, it may be
	// copied around without worrying about its size).
	std::cout << "WGPU instance: " << instance << std::endl;

	// GLFW !

	// Force X11 on linux, just to avoid to crash on wayland
	// Normally, we should let glfw choose platform automatically
	// but I made this quick & dirty fix for now waiting a better solution
	#if defined(_WIN32)
	glfwInitHint(GLFW_PLATFORM, GLFW_PLATFORM_WIN32);
	#elif defined(__linux__)
	glfwInitHint(GLFW_PLATFORM, GLFW_PLATFORM_X11); 
	#elif defined(__APPLE__)
	glfwInitHint(GLFW_PLATFORM, GLFW_PLATFORM_COCOA);
	#endif 

	if (!glfwInit()) {
		std::cerr << "Could not initialize GLFW!" << std::endl;
		return false;
 	}

	int platform = glfwGetPlatform();
    switch (platform) {
        case GLFW_PLATFORM_WAYLAND:
            printf("Running natively on Wayland\n");
            break;
        case GLFW_PLATFORM_X11:
            printf("Running on X11 (or XWayland)\n");
            break;
        case GLFW_PLATFORM_COCOA:
            printf("Running on macOS (Cocoa)\n");
            break;
        case GLFW_PLATFORM_WIN32:
            printf("Running on Windows (Win32)\n");
            break;
        default:
            printf("Running on unknown platform: %d\n", platform);
            break;
    }

	// Create window
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
	window = glfwCreateWindow(screenWidth, screenHeight, "webgpu cpp minimal", nullptr, nullptr);

	if (!window) {
		std::cerr << "Could not create window!" << std::endl;
		glfwTerminate();
		return false;
	}

	// Surface
	// Should keep alive during the app lifetime !
	surface = glfwGetWGPUSurface(instance, window);

	std::cout << "Requesting adapter..." << std::endl;

	WGPURequestAdapterOptions adapterOpts = {};
	adapterOpts.nextInChain = nullptr;
	adapterOpts.compatibleSurface = surface;
	
	WGPUAdapter adapter = requestAdapterSync(instance, &adapterOpts);

	std::cout << "Got adapter: " << adapter << std::endl;

	printInfos(adapter);

	WGPUDeviceDescriptor deviceDesc = {
		.nextInChain = nullptr,
		.label = "My Device",
		.requiredFeatureCount = 0,
		.requiredFeatures = nullptr,
		.requiredLimits = nullptr,
		.defaultQueue = {
			.nextInChain = nullptr,
			.label = "The default queue"
		},
		.deviceLostCallback = [](WGPUDeviceLostReason reason, char const* message, void* /* pUserData */) {
			std::cout << "Device lost: reason " << reason;
			if (message) std::cout << " (" << message << ")";
			std::cout << std::endl;
		}
	};

	std::cout << "Requesting device..." << std::endl;

	device = requestDeviceSync(adapter, &deviceDesc);

	std::cout << "Got device: " << device << std::endl;


	auto onDeviceError = [](WGPUErrorType type, char const* message, void* /* pUserData */) {
		std::cout << "Uncaptured device error: type " << type;
		if (message) std::cout << " (" << message << ")";
		std::cout << std::endl;
	};
	wgpuDeviceSetUncapturedErrorCallback(device, onDeviceError, nullptr /* pUserData */);

	// Display device features
	printInfos(device);

	// Surface config
	WGPUTextureFormat surfaceFormat = wgpuSurfaceGetPreferredFormat(surface, adapter);

	// We can release it now !
	wgpuInstanceRelease(instance);
	wgpuAdapterRelease(adapter);

	WGPUSurfaceConfiguration config = {
		.nextInChain = nullptr,
		.device = device,
		.format = surfaceFormat,
		.usage = WGPUTextureUsage_RenderAttachment,
		.viewFormatCount = 0,
		.viewFormats = nullptr,
		.alphaMode = WGPUCompositeAlphaMode_Auto,
		.width = screenWidth,
		.height = screenHeight,
		.presentMode = WGPUPresentMode_Fifo
	};

	wgpuSurfaceConfigure(surface, &config);

	// Get command queue
	queue = wgpuDeviceGetQueue(device);

	auto onQueueWorkDone = [](WGPUQueueWorkDoneStatus status, void* /* pUserData */) {
		std::cout << "Queued work finished with status: " << status << std::endl;
	};

	wgpuQueueOnSubmittedWorkDone(queue, onQueueWorkDone, nullptr /* pUserData */);

	// Render pipeline
	pipeline = createPipeline(device, surfaceFormat);

	return true;
}

void App::processInputs() {
    glfwPollEvents();
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, true);
    }
}

void App::renderPass(WGPUCommandEncoder encoder, WGPUTextureView targetView) {

    WGPURenderPassColorAttachment colorAttachment = {
        .nextInChain = nullptr,
        .view = targetView,
        .resolveTarget = nullptr,
        .loadOp = WGPULoadOp_Clear,
        .storeOp = WGPUStoreOp_Store,
        .clearValue = {0.0, 0.0, 0.0, 1.0},
		#ifndef WEBGPU_BACKEND_WGPU
		.depthSlice = WGPU_DEPTH_SLICE_UNDEFINED,
		#endif
    };

    WGPURenderPassDescriptor renderPassDesc = {
        .nextInChain = nullptr,
        .label = "Main Render Pass",
        .colorAttachmentCount = 1,
        .colorAttachments = &colorAttachment,
        .depthStencilAttachment = nullptr,
        .timestampWrites = nullptr,
    };

    WGPURenderPassEncoder pass = wgpuCommandEncoderBeginRenderPass(encoder, &renderPassDesc);
    {
        wgpuRenderPassEncoderSetPipeline(pass, pipeline);
        wgpuRenderPassEncoderDraw(pass, 3, 1, 0, 0); // 3 vertices, 1 instance
    }
    wgpuRenderPassEncoderEnd(pass);
    wgpuRenderPassEncoderRelease(pass);
}

void App::loop() {

	// Process inputs
    processInputs();
    if (glfwWindowShouldClose(window)) return;

	// Get target view
    WGPUTextureView targetView = getNextSurfaceTextureView(surface);
    if (!targetView) return;

    // Commands encoding
    WGPUCommandEncoderDescriptor encoderDesc = { .label = "Main Command Encoder" };
    WGPUCommandEncoder encoder = wgpuDeviceCreateCommandEncoder(device, &encoderDesc);

    renderPass(encoder, targetView);

    WGPUCommandBufferDescriptor cmdBufferDesc = { .label = "Main Command Buffer" };
    WGPUCommandBuffer command = wgpuCommandEncoderFinish(encoder, &cmdBufferDesc);
    wgpuCommandEncoderRelease(encoder);

    // Submit to GPU
    wgpuQueueSubmit(queue, 1, &command);
    wgpuCommandBufferRelease(command);

    // Cleanup frame and present
    wgpuTextureViewRelease(targetView);

	#ifndef __EMSCRIPTEN__
	wgpuSurfacePresent(surface);
	#endif

	#if defined(WEBGPU_BACKEND_DAWN)
	wgpuDeviceTick(device);
	#elif defined(WEBGPU_BACKEND_WGPU)
	wgpuDevicePoll(device, false, nullptr);
	#endif
}

bool App::isRunning() {
	return !glfwWindowShouldClose(window);
}

void App::cleanup() {    
    wgpuRenderPipelineRelease(pipeline);
    
    wgpuSurfaceUnconfigure(surface);
    wgpuSurfaceRelease(surface);
    wgpuQueueRelease(queue);
    wgpuDeviceRelease(device);
    
    // 3. Adapter & Instance (les parents)
    // wgpuAdapterRelease(adapter);
    // wgpuInstanceRelease(instance);

    glfwDestroyWindow(window);
    glfwTerminate();
}