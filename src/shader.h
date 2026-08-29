#include <iostream>
#include <fstream>
#include <sstream>

#include <webgpu/webgpu.h>

#ifdef WEBGPU_BACKEND_WGPU
#include <webgpu/wgpu.h>
#endif // WEBGPU_BACKEND_WGPU

struct Shader {

	static WGPUShaderModule createShaderModule(WGPUDevice device, const char *path) {

		// Read file
		std::ifstream fs;
    	fs.exceptions(std::ifstream::failbit | std::ifstream::badbit);

		std::string code;

		try {
			fs.open(path);
			std::stringstream shaderStream;
			shaderStream << fs.rdbuf();
			fs.close();
			
			code = shaderStream.str();

		} catch (std::ifstream::failure e) {
			std::cout << "An error occured when reading shader file " << e.what() << std::endl;
		}

		// Create WSGL descriptor
		WGPUShaderModuleWGSLDescriptor shaderCodeDesc{};
		shaderCodeDesc.chain.next = nullptr;
		shaderCodeDesc.chain.sType = WGPUSType_ShaderModuleWGSLDescriptor;
		shaderCodeDesc.code = code.c_str();

		// Create shader module descriptor
		WGPUShaderModuleDescriptor shaderDesc{};
		shaderDesc.nextInChain = &shaderCodeDesc.chain;
		shaderDesc.label = "shader_module";
		#ifdef WEBGPU_BACKEND_WGPU
		shaderDesc.hintCount = 0;
		shaderDesc.hints = nullptr;
		#endif

		// Create shader module
		WGPUShaderModule shaderModule = wgpuDeviceCreateShaderModule(device, &shaderDesc);
		return shaderModule;
	}

};