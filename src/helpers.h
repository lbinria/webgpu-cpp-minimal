#pragma once

#include <webgpu/webgpu.h>
#ifdef WEBGPU_BACKEND_WGPU
#  include <webgpu/wgpu.h>
#endif // WEBGPU_BACKEND_WGPU

#include <glfw3webgpu.h>

#ifdef __EMSCRIPTEN__
#  include <emscripten.h>
#endif // __EMSCRIPTEN__

#include <iostream>
#include <assert.h>
#include <vector>


/**
 * Utility function to get a WebGPU adapter, so that
 *     WGPUAdapter adapter = requestAdapterSync(options);
 * is roughly equivalent to
 *     const adapter = await navigator.gpu.requestAdapter(options);
 */
WGPUAdapter requestAdapterSync(WGPUInstance instance, WGPURequestAdapterOptions const * options) {
	// A simple structure holding the local information shared with the
	// onAdapterRequestEnded callback.
	struct UserData {
		WGPUAdapter adapter = nullptr;
		bool requestEnded = false;
	};
	UserData userData;

	// Callback called by wgpuInstanceRequestAdapter when the request returns
	// This is a C++ lambda function, but could be any function defined in the
	// global scope. It must be non-capturing (the brackets [] are empty) so
	// that it behaves like a regular C function pointer, which is what
	// wgpuInstanceRequestAdapter expects (WebGPU being a C API). The workaround
	// is to convey what we want to capture through the pUserData pointer,
	// provided as the last argument of wgpuInstanceRequestAdapter and received
	// by the callback as its last argument.
	auto onAdapterRequestEnded = [](WGPURequestAdapterStatus status, WGPUAdapter adapter, char const * message, void * pUserData) {
		UserData& userData = *reinterpret_cast<UserData*>(pUserData);
		if (status == WGPURequestAdapterStatus_Success) {
			userData.adapter = adapter;
		} else {
			std::cout << "Could not get WebGPU adapter: " << message << std::endl;
		}
		userData.requestEnded = true;
	};

	// Call to the WebGPU request adapter procedure
	wgpuInstanceRequestAdapter(
		instance /* equivalent of navigator.gpu */,
		options,
		onAdapterRequestEnded,
		(void*)&userData
	);

	// We wait until userData.requestEnded gets true
	// [...] Wait for request to end

	assert(userData.requestEnded);

	return userData.adapter;
}

/**
 * Utility function to get a WebGPU device, so that
 *     WGPUDevice device = requestDeviceSync(adapter, options);
 * is roughly equivalent to
 *     const device = await adapter.requestDevice(descriptor);
 * It is very similar to requestAdapter
 */
WGPUDevice requestDeviceSync(WGPUAdapter adapter, WGPUDeviceDescriptor const * descriptor) {
	struct UserData {
		WGPUDevice device = nullptr;
		bool requestEnded = false;
	};
	UserData userData;

	auto onDeviceRequestEnded = [](WGPURequestDeviceStatus status, WGPUDevice device, char const * message, void * pUserData) {
		UserData& userData = *reinterpret_cast<UserData*>(pUserData);
		if (status == WGPURequestDeviceStatus_Success) {
			userData.device = device;
		} else {
			std::cout << "Could not get WebGPU device: " << message << std::endl;
		}
		userData.requestEnded = true;
	};

	wgpuAdapterRequestDevice(
		adapter,
		descriptor,
		onDeviceRequestEnded,
		(void*)&userData
	);

#ifdef __EMSCRIPTEN__
	while (!userData.requestEnded) {
		emscripten_sleep(100);
	}
#endif // __EMSCRIPTEN__

	assert(userData.requestEnded);

	return userData.device;
}

WGPUTextureView getNextSurfaceTextureView(WGPUSurface surface) {
	// Get the surface texture
	WGPUSurfaceTexture surfaceTexture;
	wgpuSurfaceGetCurrentTexture(surface, &surfaceTexture);
	if (surfaceTexture.status != WGPUSurfaceGetCurrentTextureStatus_Success) {
		return nullptr;
	}

	// Create a view for this surface texture
	WGPUTextureViewDescriptor viewDescriptor;
	viewDescriptor.nextInChain = nullptr;
	viewDescriptor.label = "Surface texture view";
	viewDescriptor.format = wgpuTextureGetFormat(surfaceTexture.texture);
	viewDescriptor.dimension = WGPUTextureViewDimension_2D;
	viewDescriptor.baseMipLevel = 0;
	viewDescriptor.mipLevelCount = 1;
	viewDescriptor.baseArrayLayer = 0;
	viewDescriptor.arrayLayerCount = 1;
	viewDescriptor.aspect = WGPUTextureAspect_All;
	WGPUTextureView targetView = wgpuTextureCreateView(surfaceTexture.texture, &viewDescriptor);

#ifndef WEBGPU_BACKEND_WGPU
	// We no longer need the texture, only its view
	// (NB: with wgpu-native, surface textures must not be manually released)
	wgpuTextureRelease(surfaceTexture.texture);
#endif // WEBGPU_BACKEND_WGPU

	return targetView;
}

// const char* vendorName(uint32_t vendorID)
// {
//     switch (vendorID) {
//         case 0x10DE:
//             return "NVIDIA";

//         case 0x1002:
//             return "AMD";

//         case 0x8086:
//             return "Intel";

//         case 0x106B:
//             return "Apple";

//         case 0x13B5:
//             return "Arm";

//         case 0x5143:
//             return "Qualcomm";

//         case 0x1414:
//             return "Microsoft / software adapter";

//         case 0x10005:
//             return "Mesa software renderer";

//         default:
//             return "Unknown";
//     }
// }

const char* featureName(WGPUFeatureName feature)
{
    switch (feature) {
        case WGPUFeatureName_DepthClipControl:
            return "DepthClipControl";

        case WGPUFeatureName_Depth32FloatStencil8:
            return "Depth32FloatStencil8";

        case WGPUFeatureName_TimestampQuery:
            return "TimestampQuery";

        case WGPUFeatureName_TextureCompressionBC:
            return "TextureCompressionBC";

        case WGPUFeatureName_TextureCompressionETC2:
            return "TextureCompressionETC2";

        case WGPUFeatureName_TextureCompressionASTC:
            return "TextureCompressionASTC";

        case WGPUFeatureName_IndirectFirstInstance:
            return "IndirectFirstInstance";

        case WGPUFeatureName_ShaderF16:
            return "ShaderF16";

        case WGPUFeatureName_RG11B10UfloatRenderable:
            return "RG11B10UfloatRenderable";

        case WGPUFeatureName_BGRA8UnormStorage:
            return "BGRA8UnormStorage";

        case WGPUFeatureName_Float32Filterable:
            return "Float32Filterable";

        default:
            return "Unknown";
    }
}

const char* backendName(WGPUBackendType backend)
{
    switch (backend) {
        case WGPUBackendType_Null:
            return "Null";

        case WGPUBackendType_WebGPU:
            return "WebGPU";

        case WGPUBackendType_D3D11:
            return "Direct3D 11";

        case WGPUBackendType_D3D12:
            return "Direct3D 12";

        case WGPUBackendType_Metal:
            return "Metal";

        case WGPUBackendType_Vulkan:
            return "Vulkan";

        case WGPUBackendType_OpenGL:
            return "OpenGL";

        case WGPUBackendType_OpenGLES:
            return "OpenGL ES";

        default:
            return "Unknown";
    }
}

void printInfos(WGPUAdapter adapter) {

	WGPUAdapterProperties properties = {};
	properties.nextInChain = nullptr;
	wgpuAdapterGetProperties(adapter, &properties);

	// Display adapter properties
	std::cout << "Adapter properties:" << std::endl;
	std::cout << " - vendorID: " << properties.vendorID << std::endl;

	std::cout << " - vendorName: " << (properties.vendorName ? properties.vendorName : "unknown") << std::endl;

	if (properties.architecture) {
		std::cout << " - architecture: " << properties.architecture << std::endl;
	}

	std::cout << " - deviceID: " << properties.deviceID << std::endl;
	std::cout << " - Adapter: " << (properties.name ? properties.name : "unknown") << std::endl;

	if (properties.driverDescription) {
		std::cout << " - driverDescription: " << properties.driverDescription << std::endl;
	}

	// std::cout << std::hex;
	// std::cout << " - adapterType: 0x" << properties.adapterType << std::endl;
	// std::cout << " - backendType: 0x" << properties.backendType << std::endl;
	// std::cout << std::dec; // Restore decimal numbers

	
	std::cout << "Backend: " << backendName(properties.backendType) << std::endl;

	std::vector<WGPUFeatureName> features;

	// Call the function a first time with a null return address, just to get
	// the entry count.
	size_t featureCount = wgpuAdapterEnumerateFeatures(adapter, nullptr);

	// Allocate memory (could be a new, or a malloc() if this were a C program)
	features.resize(featureCount);

	// Call the function a second time, with a non-null return address
	wgpuAdapterEnumerateFeatures(adapter, features.data());

	// Display adapter features
	std::cout << "Adapter features:" << std::endl;
	std::cout << std::hex; // Write integers as hexadecimal to ease comparison with webgpu.h literals
	for (auto f : features) {
		std::cout << " - " << "0x" << static_cast<unsigned int>(f) << "-" << featureName(f) << std::endl;
	}
	std::cout << std::dec; // Restore decimal numbers


	WGPUSupportedLimits supportedLimits = {};
	supportedLimits.nextInChain = nullptr;
	bool success = wgpuAdapterGetLimits(adapter, &supportedLimits); 

	// Display adapter limits
	if (success) {
		std::cout << "Adapter limits:" << std::endl;
		std::cout << " - maxTextureDimension1D: " << supportedLimits.limits.maxTextureDimension1D << std::endl;
		std::cout << " - maxTextureDimension2D: " << supportedLimits.limits.maxTextureDimension2D << std::endl;
		std::cout << " - maxTextureDimension3D: " << supportedLimits.limits.maxTextureDimension3D << std::endl;
		std::cout << " - maxTextureArrayLayers: " << supportedLimits.limits.maxTextureArrayLayers << std::endl;
	}
}

// We also add an inspect device function:
void printInfos(WGPUDevice device) {
	std::vector<WGPUFeatureName> features;
	size_t featureCount = wgpuDeviceEnumerateFeatures(device, nullptr);
	features.resize(featureCount);
	wgpuDeviceEnumerateFeatures(device, features.data());

	std::cout << "Device features:" << std::endl;
	std::cout << std::hex;
	for (auto f : features) {
		std::cout << " - 0x" << f << std::endl;
	}
	std::cout << std::dec;

	WGPUSupportedLimits limits = {};
	limits.nextInChain = nullptr;

#ifdef WEBGPU_BACKEND_DAWN
	bool success = wgpuDeviceGetLimits(device, &limits) == WGPUStatus_Success;
#else
	bool success = wgpuDeviceGetLimits(device, &limits);
#endif

	if (success) {
		std::cout << "Device limits:" << std::endl;
		std::cout << " - maxTextureDimension1D: " << limits.limits.maxTextureDimension1D << std::endl;
		std::cout << " - maxTextureDimension2D: " << limits.limits.maxTextureDimension2D << std::endl;
		std::cout << " - maxTextureDimension3D: " << limits.limits.maxTextureDimension3D << std::endl;
		std::cout << " - maxTextureArrayLayers: " << limits.limits.maxTextureArrayLayers << std::endl;
		// [...] Extra device limits
	}
}