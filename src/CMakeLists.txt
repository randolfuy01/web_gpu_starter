cmake_minimum_required(VERSION 3.0...3.25)
project(
    WebGpuStarter
    VERSION 0.1.0 
    LANGUAGES CXX C 
)

add_subdirectory(glfw)
add_subdirectory(webgpu)
add_subdirectory(glfw3webgpu)

add_executable(App main.cpp)
target_link_libraries(App PRIVATE webgpu glfw glfw3webgpu)


set_target_properties(App PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
    COMPILE_WARNING_AS_ERROR ON
)

target_copy_webgpu_binaries(App)

# Options that are specific to Emscripten
if (DAWN)
    # Generate a full web page rather than a simple WebAssembly module
    set_target_properties(App PROPERTIES SUFFIX ".html")
endif()