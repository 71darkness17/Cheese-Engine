#pragma once

#define GLFW_INCLUDE_VULKAN
#define GLM_FORCE_RADIANS

#include <GLFW/glfw3.h>
#include <external/stb_image.h>

#include <algorithm>
#include <array>
#include <chrono>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>
#include <mutex>
#include <optional>
#include <set>
#include <stdexcept>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

#include "renderQueue.hpp"

const uint32_t HEIGHT           = 600;
const uint32_t WIDTH            = 800;
const int MAX_FRAMES_IN_FLIGHT  = 2;
const size_t MAX_VERTICES       = 1200;
const size_t MAX_INDICES        = 3600;
const uint32_t MAX_IMAGE_ARRAYS = 128;

const std::vector<char*> validationLayers = {"VK_LAYER_KHRONOS_validation"};

const std::vector<const char*> deviceExtensions = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};

enum KeyStatusEnum { KEY_PRESSED, KEY_RELEASED };

#ifdef NDEBUG
const bool enableValidationLayers = false;
#else
const bool enableValidationLayers = true;
#endif

class GraphicCore {
public:
  GraphicCore(GLFWwindow* window);

  uint32_t addRectangle(glm::vec2 position, float width, float height, glm::vec3 color);
  uint32_t addTriangle(std::array<glm::vec2, 3> positions, glm::vec3 color);

  void removeFigure(uint32_t index);

  void setTransform(uint32_t index, const glm::mat4& transform);

  void setCamera(glm::vec2 position, float zoom);

  TextureDescriptor addTexture(const std::string& path);

  void setTexture(uint32_t figureIndex, TextureDescriptor textureDescriptor,
                  const std::vector<glm::vec2>& texCoords);

  void startGraphicThread();
  void stopGraphicThread();

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

  struct UniformBufferObject {
    alignas(16) glm::mat4 view;
    alignas(16) glm::mat4 proj;
  };

  struct PushConstant {
    alignas(16) glm::mat4 model;
    uint32_t textureHandler;
  };

  struct FigureDesc {
    size_t vertexOffset;
    uint32_t vertexCount;
    size_t firstIndex;
    uint32_t indexCount;
    glm::mat4 model;
    uint32_t textureHandler = 0;
  };

  struct Camera {
    glm::vec2 position;
    float zoom;
  };

  struct Vertex {
    glm::vec2 pos;
    glm::vec3 color;
    glm::vec2 texCoord;

    static VkVertexInputBindingDescription getBindingDescription();

    static std::array<VkVertexInputAttributeDescription, 3> getAttributeDescriptions();
  };

  struct TextureArray {
    VkImage image;
    VkDeviceMemory imageMemory;
    VkImageView imageView;
    VkSampler sampler;
    uint32_t layersCount;
    VkExtent2D extent;
    std::vector<stbi_uc*> pixels;
  };

  std::mutex verticesMutex;
  std::mutex stopMutex;
  std::thread graphicThread;

  GLFWwindow* window;
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
  size_t currentFrame     = 0;
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
  std::vector<void*> uniformBuffersMapped;
  VkDescriptorPool descriptorPool;
  std::vector<VkDescriptorSet> descriptorSets;
  std::vector<Vertex> vertices;
  std::vector<uint16_t> indices;
  std::unordered_map<int, FigureDesc> figures;
  bool isStopped         = false;
  uint32_t indicesToDraw = 0;
  uint32_t verticesCount = 0;
  uint32_t nextFigureHex = 0x0;
  RenderQueue renderQueue;
  Camera camera;
  std::vector<std::string> texturePaths;
  std::vector<TextureArray> textureArrays;

  static VkResult CreateDebugUtilsMessengerEXT(
    VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
    const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger);

  static void DestroyDebugUtilsMessengerEXT(VkInstance instance,
                                            VkDebugUtilsMessengerEXT debugMessenger,
                                            const VkAllocationCallbacks* pAllocator);

  static std::vector<char> readFile(const std::string& filename);

  void run();

  void initWindow();

  void initVulkan();

  void createSurface();

  void mainLoop();

  static void framebufferSizeCallback(GLFWwindow* window, int width, int height);

  void _addRectangle(glm::vec2 position, float width, float height, glm::vec3 color,
                     uint32_t index);

  void _addTriangle(std::array<glm::vec2, 3> positions, glm::vec3 color, uint32_t index);

  void _removeFigure(uint32_t index);

  void _setTranform(uint32_t index, glm::mat4 transform);

  void _setCamera(glm::vec2 position, float zoom);

  void _setTexture(uint32_t figureIndex, TextureDescriptor textureDescriptor,
                   std::vector<glm::vec2> texCoords);

  void addDefaultTexture();

  void pollRenderQueue();

  void drawFrame();

  void cleanup();

  bool checkExtensionsSupport(const uint32_t extCount, const char** extToCheck);

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

  void createImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling,
                   VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image,
                   VkDeviceMemory& imageMemory, uint32_t arrayLayers);

  VkCommandBuffer beginSingleTimeCommands();

  void endSingleTimeCommands(VkCommandBuffer commandBuffer);

  void createTextureImage();

  void createTextureImageView();

  VkImageView createImageView(VkImage image, VkFormat format, uint32_t layerCount,
                              VkImageViewType viewType);

  void createTextureSampler();

  void transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout,
                             VkImageLayout newLayout, uint32_t layerCount);

  void copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height,
                         uint32_t layerCount);

  void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties,
                    VkBuffer& buffer, VkDeviceMemory& bufferMemory);

  void createVertexBuffer();

  void createIndexBuffer();

  void createUniformBuffers();

  void updateUniformBuffer(uint32_t currentImage);

  void updateVertexBuffer();

  void createDescriptorPool();

  void createDescriptorSets();

  void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize bufferSize);

  uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);

  void recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex);

  void createCommandBuffers();

  void createSyncObjects();

  VkShaderModule createShaderModule(const std::vector<char>& code);

  bool checkDeviceExtensionSupport(VkPhysicalDevice device);

  bool isDeviceSuitable(VkPhysicalDevice device);

  void pickPhysicalDevice();

  QueueFamilyIndicies findQueueFamilies(VkPhysicalDevice device);

  SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device);

  VkSurfaceFormatKHR chooseSwapSurfaceFormat(
    const std::vector<VkSurfaceFormatKHR>& availableFormats);

  VkPresentModeKHR chooseSwapPresentMode(
    const std::vector<VkPresentModeKHR>& availablePresentModes);

  VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);

  std::vector<const char*> getRequiredExtensions();

  void createInstance();

  bool checkValidationLayerSupport();

  static VKAPI_ATTR VkBool32 VKAPI_CALL
  debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                VkDebugUtilsMessageTypeFlagsEXT messageType,
                const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData);

  void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo);

  void setupDebugMessenger();
};
