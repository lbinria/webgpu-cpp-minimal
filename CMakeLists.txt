cmake_minimum_required(VERSION 4.2.3)
project(webgpu_template CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_BUILD_RPATH_USE_ORIGIN TRUE)
# set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)

# Set Release as default config for multi-config
get_property(is_multi_config GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG)
if(NOT is_multi_config AND NOT (CMAKE_BUILD_TYPE OR DEFINED ENV{CMAKE_BUILD_TYPE}))
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Release default")
endif()

if (MSVC)
    add_compile_options(/W4 /openmp:llvm   /wd26451 /bigobj)
else()
  # TODO activate when ready to that 
  # lots of warnings and all warnings as errors
  # add_compile_options(-Wall -Wextra -pedantic)
endif()


if (NOT EMSCRIPTEN)
    # Add the 'glfw' directory, which contains the definition of a 'glfw' target
    add_subdirectory(third_party/glfw)
else()
    # Create a mock 'glfw' target that just sets the `-sUSE_GLFW=3` link option:
    add_library(glfw INTERFACE)
    target_link_options(glfw INTERFACE -sUSE_GLFW=3)
endif()


add_subdirectory(third_party/WebGPU/WebGPU-distribution-wgpu-v0.19.4.1)
add_subdirectory(third_party/glfw3webgpu)

file(GLOB_RECURSE PROJECT_SOURCES CONFIGURE_DEPENDS
    "${PROJECT_SOURCE_DIR}/src/*.cpp"
    "${PROJECT_SOURCE_DIR}/src/*.cxx"
)

file(GLOB_RECURSE PROJECT_HEADERS CONFIGURE_DEPENDS
    "${PROJECT_SOURCE_DIR}/src/*.h"
    "${PROJECT_SOURCE_DIR}/src/*.hpp"
)

add_executable(${PROJECT_NAME}
    ${PROJECT_SOURCES}
    ${PROJECT_HEADERS}
)

target_include_directories(${PROJECT_NAME} PRIVATE
    "${PROJECT_SOURCE_DIR}/src"
)

# Add the 'webgpu' target as a dependency of our App
# In both cases, we can now link to the 'glfw' target
target_link_libraries(${PROJECT_NAME} PRIVATE glfw webgpu glfw3webgpu)

if (NOT WIN32)
    target_link_libraries(${PROJECT_NAME} PRIVATE m)
endif()

# Copy WebGPU runtime files required by your executable into its output directory after building
target_copy_webgpu_binaries(${PROJECT_NAME})

message("Copying shaders from ${PROJECT_SOURCE_DIR}/shaders to ${CMAKE_CURRENT_BINARY_DIR}/shaders")

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${PROJECT_SOURCE_DIR}/shaders"
        "$<TARGET_FILE_DIR:${PROJECT_NAME}>/shaders"

    COMMENT "Copying shaders to output directory"
)