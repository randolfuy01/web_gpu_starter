#include <iostream>
#include <webgpu/webgpu.h>
#include <GLFW/glfw3.h>
#include <cassert>
#include <vector>

/**
 * Utility function to get a WebGPU adapter, so that
 *     WGPUAdapter adapter = requestAdapterSync(options);
 * is roughly equivalent to
 *     const adapter = await navigator.gpu.requestAdapter(options);
 */
WGPUAdapter requestAdapterSync(WGPUInstance instance, WGPURequestAdapterOptions const *options)
{
    // A simple structure holding the local information shared with the
    // onAdapterRequestEnded callback.
    struct UserData
    {
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
    auto onAdapterRequestEnded = [](WGPURequestAdapterStatus status, WGPUAdapter adapter, char const *message, void *pUserData)
    {
        UserData &userData = *reinterpret_cast<UserData *>(pUserData);
        if (status == WGPURequestAdapterStatus_Success)
        {
            userData.adapter = adapter;
        }
        else
        {
            std::cout << "Could not get WebGPU adapter: " << message << std::endl;
        }
        userData.requestEnded = true;
    };

    // Call to the WebGPU request adapter procedure
    wgpuInstanceRequestAdapter(
        instance /* equivalent of navigator.gpu */,
        options,
        onAdapterRequestEnded,
        (void *)&userData);

    assert(userData.requestEnded);

    return userData.adapter;
}

/**
 * Utility function to get a WebGPU device, so that
 *     WGPUAdapter device = requestDeviceSync(adapter, options);
 * is roughly equivalent to
 *     const device = await adapter.requestDevice(descriptor);
 * It is very similar to requestAdapter
 */
WGPUDevice requestDeviceSync(WGPUAdapter adapter, WGPUDeviceDescriptor const *descriptor)
{
    struct UserData
    {
        WGPUDevice device = nullptr;
        bool requestEnded = false;
    };
    UserData userData;

    auto onDeviceRequestEnded = [](WGPURequestDeviceStatus status, WGPUDevice device, char const *message, void *pUserData)
    {
        UserData &userData = *reinterpret_cast<UserData *>(pUserData);
        if (status == WGPURequestDeviceStatus_Success)
        {
            userData.device = device;
        }
        else
        {
            std::cout << "Could not get WebGPU device: " << message << std::endl;
        }
        userData.requestEnded = true;
    };

    wgpuAdapterRequestDevice(
        adapter,
        descriptor,
        onDeviceRequestEnded,
        (void *)&userData);

    assert(userData.requestEnded);

    return userData.device;
}

// We also add an inspect device function:
void inspectDevice(WGPUDevice device)
{
    std::vector<WGPUFeatureName> features;
    size_t featureCount = wgpuDeviceEnumerateFeatures(device, nullptr);
    features.resize(featureCount);
    wgpuDeviceEnumerateFeatures(device, features.data());

    std::cout << "Device features:" << std::endl;
    std::cout << std::hex;
    for (auto f : features)
    {
        std::cout << " - 0x" << f << std::endl;
    }
    std::cout << std::dec;

    WGPUSupportedLimits limits = {};
    limits.nextInChain = nullptr;

    bool success = wgpuDeviceGetLimits(device, &limits) == WGPUStatus_Success;

    if (success)
    {
        std::cout << "Device limits:" << std::endl;
        std::cout << " - maxTextureDimension1D: " << limits.limits.maxTextureDimension1D << std::endl;
        std::cout << " - maxTextureDimension2D: " << limits.limits.maxTextureDimension2D << std::endl;
        std::cout << " - maxTextureDimension3D: " << limits.limits.maxTextureDimension3D << std::endl;
        std::cout << " - maxTextureArrayLayers: " << limits.limits.maxTextureArrayLayers << std::endl;
    }
}

// Initializing
int createWindow()
{
    glfwInit();

    if (!glfwInit()) {
        std::cerr << "Could not initialize GLFW!" << std::endl;
        return 1;
    }

    GLFWwindow* window = glfwCreateWindow(640, 480, "WebGPU", nullptr, nullptr);

    if (!window) {
        std::cerr << "Could not open window!" << std::endl;
        glfwTerminate();
        return 1;
    }

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
    }

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}

int main(int, char **)
{
    std::cout << "Using webgpu!" << std::endl;
    WGPUInstanceDescriptor desc = {};
    desc.nextInChain = nullptr;

    std::cout << "Starting webgpu instance!" << std::endl;
    WGPUInstance instance = wgpuCreateInstance(&desc);

    if (!instance)
    {
        std::cerr << "Could not initialize WebGPU!" << std::endl;
        return 1;
    }

    std::cout << "WGPU instance: " << instance << std::endl;

    // Working with the adapters
    std::cout << "Requesting adapter..." << std::endl;
    WGPURequestAdapterOptions adapterOpts = {};
    adapterOpts.nextInChain = nullptr;
    WGPUAdapter adapter = requestAdapterSync(instance, &adapterOpts);
    std::cout << "Got adapter: " << adapter << std::endl;

    // Limits
    WGPUSupportedLimits supportedLimits = {};
    supportedLimits.nextInChain = nullptr;
    bool success = wgpuAdapterGetLimits(adapter, &supportedLimits) == WGPUStatus_Success;

    if (success)
    {
        std::cout << "Adapter limits:" << std::endl;
        std::cout << " - maxTextureDimension1D: " << supportedLimits.limits.maxTextureDimension1D << std::endl;
        std::cout << " - maxTextureDimension2D: " << supportedLimits.limits.maxTextureDimension2D << std::endl;
        std::cout << " - maxTextureDimension3D: " << supportedLimits.limits.maxTextureDimension3D << std::endl;
        std::cout << " - maxTextureArrayLayers: " << supportedLimits.limits.maxTextureArrayLayers << std::endl;
    }

    // Features
    std::vector<WGPUFeatureName> features;

    // Call the function to get entry count and allocate memory
    size_t featureCount = wgpuAdapterEnumerateFeatures(adapter, nullptr);
    features.resize(featureCount);

    wgpuAdapterEnumerateFeatures(adapter, features.data());
    std::cout << "Adapter features:" << std::endl;
    std::cout << std::hex;
    for (auto f : features)
    {
        std::cout << " - 0x" << f << std::endl;
    }
    std::cout << std::dec;

    // Properties
    WGPUAdapterProperties properties = {};
    properties.nextInChain = nullptr;
    wgpuAdapterGetProperties(adapter, &properties);
    std::cout << "Adapter properties:" << std::endl;
    std::cout << " - vendorID: " << properties.vendorID << std::endl;
    if (properties.vendorName)
    {
        std::cout << " - vendorName: " << properties.vendorName << std::endl;
    }
    if (properties.architecture)
    {
        std::cout << " - architecture: " << properties.architecture << std::endl;
    }
    std::cout << " - deviceID: " << properties.deviceID << std::endl;
    if (properties.name)
    {
        std::cout << " - name: " << properties.name << std::endl;
    }
    if (properties.driverDescription)
    {
        std::cout << " - driverDescription: " << properties.driverDescription << std::endl;
    }
    std::cout << std::hex;
    std::cout << " - adapterType: 0x" << properties.adapterType << std::endl;
    std::cout << " - backendType: 0x" << properties.backendType << std::endl;
    std::cout << std::dec;

    // Device
    std::cout << "Requesting device..." << std::endl;
    WGPUDeviceDescriptor deviceDesc = {};
    WGPUDevice device = requestDeviceSync(adapter, &deviceDesc);

    std::cout << "Got device: " << device << std::endl;
    deviceDesc.nextInChain = nullptr;
    deviceDesc.label = "My Device";      // anything works here, that's your call
    deviceDesc.requiredFeatureCount = 0; // we do not require any specific feature
    deviceDesc.requiredLimits = nullptr; // we do not require any specific limit
    deviceDesc.defaultQueue.nextInChain = nullptr;
    deviceDesc.defaultQueue.label = "The default queue";
    deviceDesc.deviceLostCallback = nullptr;

    // Inspect the device
    inspectDevice(device);

    // Window
    int window = createWindow();
    return 0;
}
