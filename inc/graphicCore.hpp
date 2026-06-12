#pragma once

#define GLFW_INCLUDE_VULKAN
#define GLM_FORCE_RADIANS

#include "renderQueue.hpp"

#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <chrono>
#include <vector>
#include <cstring>
#include <iostream>
#include <cstdlib>
#include <stdexcept>
#include <optional>
#include <set>
#include <algorithm>
#include <fstream>
#include <array>
#include <unordered_map>
#include <mutex>
#include <thread>

const uint32_t HEIGHT = 600;
const uint32_t WIDTH = 800;
const int MAX_FRAMES_IN_FLIGHT = 2;
const size_t MAX_VERTICES = 1200;
const size_t MAX_INDICES = 3600;

const std::vector<char *> validationLayers = {
    "VK_LAYER_KHRONOS_validation"
};

const std::vector<const char*> deviceExtensions = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

enum KeyStatusEnum {
    KEY_PRESSED,
    KEY_RELEASED
};

#ifdef NDEBUG
    const bool enableValidationLayers = false;
#else
    const bool enableValidationLayers = true;
#endif


class GraphicCore {
public:

    GraphicCore(GLFWwindow *window);



    uint32_t addRectangle(glm::vec2 position,
                          float width,
                          float height,
                          glm::vec3 color);
    uint32_t addTriangle(std::array<glm::vec2, 3> positions,
                         glm::vec3 color);

    void removeFigure(uint32_t index);

    void startGraphicThread();
    void stopGraphicThread();

    struct Vertex {
        glm::vec2 pos;
        glm::vec3 color;

        static VkVertexInputBindingDescription getBindingDescription();

        static std::array<VkVertexInputAttributeDescription, 2> getAttributeDescriptions();
    };

    struct FigureDesc {
        size_t vertexOffset;
        size_t firstIndex;
        uint32_t indexCount;
        glm::mat4 model;
    };

private:
    struct QueueFamilyIndicies {
        std::optional<uint32_t> graphicFamily;
        std::optional<uint32_t> presentFamily;
        
        bool isComplete();
    };

    struct SwapChainSupportDetails {
        VkSurfaceCapabilitiesKHR capabilities;
        std::vector<VkSurfaceFormatKHR> formats;
        std::vector<VkPresentModeKHR> presentModes;
    };

    struct UniformBufferObject  {
        alignas(16) glm::mat4 model;
        alignas(16) glm::mat4 view;
        alignas(16) glm::mat4 proj;
    };

    std::mutex verticesMutex;
    std::mutex stopMutex;
    std::thread graphicThread;

    GLFWwindow *window;
    VkInstance instance;
    std::vector<VkExtensionProperties> instanceExtensions;
    VkDebugUtilsMessengerEXT debugMessenger;
    VkSurfaceKHR surface;
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    VkDevice device;
    VkQueue graphicsQueue;
    VkQueue presentQueue;
    VkSwapchainKHR swapChain = VK_NULL_HANDLE;
    std::vector<VkImage> swapChainImages;
    VkFormat swapChainImageFormat;
    VkExtent2D swapChainExtent;
    std::vector<VkImageView> swapChainImageViews;
    VkRenderPass renderPass;
    VkDescriptorSetLayout descriptorSetLayout;
    VkPipelineLayout pipelineLayout;
    VkPipeline graphicsPipeline;
    std::vector<VkFramebuffer> swapChainFramebuffers;
    VkCommandPool commandPool;
    std::vector<VkCommandBuffer> commandBuffers;
    std::vector<VkSemaphore> imageAvailableSemaphores;
    std::vector<VkSemaphore> renderFinishedSemaphores;
    std::vector<VkFence> inFlightFences;
    std::vector<VkFence> imagesInFlight;
    size_t currentFrame = 0;
    bool framebufferResized = false;
    VkBuffer vertexBuffer;
    VkDeviceMemory vertexBufferMemory;
    bool verticesChanged = false;
    VkBuffer stagingVertexBuffer;
    VkDeviceMemory stagingVertexBufferMemory;
    VkBuffer indexBuffer;
    VkDeviceMemory indexBufferMemory;
    VkBuffer stagingIndexBuffer;
    VkDeviceMemory stagingIndexBufferMemory;
    std::vector<VkBuffer> uniformBuffers;
    std::vector<VkDeviceMemory> uniformBuffersMemory;
    std::vector<void *> uniformBuffersMapped;
    VkDescriptorPool descriptorPool;
    std::vector<VkDescriptorSet> descriptorSets;
    std::vector<Vertex> vertices;
    std::vector<uint16_t> indices;
    std::unordered_map<int, FigureDesc> figures;
    bool isStopped = false;
    uint32_t indicesToDraw = 0;
    uint32_t verticesCount = 0;
    uint32_t nextFigureHex = 0x0;
    RenderQueue renderQueue;

    static VkResult CreateDebugUtilsMessengerEXT(VkInstance instance,
                                                 const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
                                                 const VkAllocationCallbacks* pAllocator,
                                                 VkDebugUtilsMessengerEXT* pDebugMessenger);

    static void DestroyDebugUtilsMessengerEXT(VkInstance instance,
                                              VkDebugUtilsMessengerEXT debugMessenger,
                                              const VkAllocationCallbacks* pAllocator);

    static std::vector<char> readFile(const std::string &filename);

    void run();

    void initWindow();

    void initVulkan();

    void createSurface();

    void mainLoop();

    static void framebufferSizeCallback(GLFWwindow* window, int width, int height);

    void _addRectangle(glm::vec2 position,
                       float width,
                       float height,
                       glm::vec3 color,
                       uint32_t index);

    void _addTriangle(std::array<glm::vec2, 3> positions,
                      glm::vec3 color,
                      uint32_t index);

    void _removeFigure(uint32_t index);

    void pollRenderQueue();

    void drawFrame();

    void cleanup();

    bool checkExtensionsSupport(const uint32_t extCount, const char ** extToCheck);

    void createLogicalDevice();

    void createSwapChain();

    void cleanupSwapChain();
    
    void recreateSwapChain();

    void createImageViews();

    void createRenderPass();

    void createDescriptorSetLayout();

    void createGraphicsPipeline();

    void createFrameBuffers();

    void createCommandPool();

    void createBuffer(VkDeviceSize size,
                      VkBufferUsageFlags usage, 
                      VkMemoryPropertyFlags properties, 
                      VkBuffer& buffer, 
                      VkDeviceMemory& bufferMemory);

    void createVertexBuffer();

    void createIndexBuffer();

    void createUniformBuffers();

    void updateUniformBuffer(uint32_t currentImage);

    void updateVertexBuffer();

    void createDescriptorPool();

    void createDescriptorSets();

    void copyBuffer(VkBuffer srcBuffer,
                    VkBuffer dstBuffer,
                    VkDeviceSize bufferSize);

    uint32_t findMemoryType(uint32_t typeFilter,
                            VkMemoryPropertyFlags properties);

    void recordCommandBuffer(VkCommandBuffer commandBuffer,
                             uint32_t imageIndex);

    void createCommandBuffers();

    void createSyncObjects();

    VkShaderModule createShaderModule(const std::vector<char>& code);

    bool checkDeviceExtensionSupport(VkPhysicalDevice device);

    bool isDeviceSuitable(VkPhysicalDevice device);

    void pickPhysicalDevice();

    QueueFamilyIndicies findQueueFamilies(VkPhysicalDevice device);

    SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device);

    VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR> &availableFormats);

    VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR> &availablePresentModes);

    VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR &capabilities);

    std::vector<const char *> getRequiredExtensions();

    void createInstance();

    bool checkValidationLayerSupport();

    static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
        VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
        VkDebugUtilsMessageTypeFlagsEXT messageType,
        const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
        void* pUserData);

    void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo);

    void setupDebugMessenger();
    
};
