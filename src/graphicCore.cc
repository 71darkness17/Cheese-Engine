#include "graphicCore.hpp"

#define STB_IMAGE_IMPLEMENTATION
#include <external/stb_image.h>

/* Graphics Api start/stop definition */

/* --- */

VkResult GraphicCore::CreateDebugUtilsMessengerEXT(
  VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
  const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger) {
  auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
    instance, "vkCreateDebugUtilsMessengerEXT");
  if (func != nullptr) {
    return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
  } else {
    return VK_ERROR_EXTENSION_NOT_PRESENT;
  }
}

void GraphicCore::DestroyDebugUtilsMessengerEXT(VkInstance instance,
                                                VkDebugUtilsMessengerEXT debugMessenger,
                                                const VkAllocationCallbacks* pAllocator) {
  auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
    instance, "vkDestroyDebugUtilsMessengerEXT");
  if (func != nullptr) {
    func(instance, debugMessenger, pAllocator);
  }
}

GraphicCore::GraphicCore(GLFWwindow* window)
  : window(window), vertices(MAX_VERTICES), indices(MAX_INDICES) {
  camera.position = {WIDTH / 2.0f, HEIGHT / 2.0f};
  camera.zoom     = 1.0f;
  addDefaultTexture();
}

void GraphicCore::run() {
  initWindow();
  initVulkan();
  mainLoop();
  cleanup();
}

uint32_t GraphicCore::addRectangle(glm::vec2 position, float width, float height, glm::vec3 color) {
  RenderCommand cmd;
  cmd.type = AddRect;
  cmd.data = RenderCommand::AddRect{
    .position = position, .width = width, .height = height, .color = color, .index = nextFigureHex};
  renderQueue.push(cmd);
  return nextFigureHex++;
}

void GraphicCore::_addRectangle(glm::vec2 position, float width, float height, glm::vec3 color,
                                uint32_t index) {
  std::lock_guard<std::mutex> verticesLock(verticesMutex);
  size_t startNum           = verticesCount;
  vertices[verticesCount++] = {position, color};
  vertices[verticesCount++] = {position + glm::vec2(width, 0), color};
  vertices[verticesCount++] = {position + glm::vec2(width, height), color};
  vertices[verticesCount++] = {position + glm::vec2(0, height), color};

  size_t startIndex        = indicesToDraw;
  indices[indicesToDraw++] = startNum;
  indices[indicesToDraw++] = startNum + 1;
  indices[indicesToDraw++] = startNum + 2;
  indices[indicesToDraw++] = startNum + 2;
  indices[indicesToDraw++] = startNum + 3;
  indices[indicesToDraw++] = startNum;

  figures[index] = {.vertexOffset = startNum,
                    .vertexCount  = 4,
                    .firstIndex   = startIndex,
                    .indexCount   = 6,
                    .model        = glm::mat4(1.0f)};

  verticesChanged = true;
}

uint32_t GraphicCore::addTriangle(std::array<glm::vec2, 3> positions, glm::vec3 color) {
  RenderCommand cmd;
  cmd.type = AddTriangle;
  cmd.data = RenderCommand::AddTri{.positions = positions, .color = color, .index = nextFigureHex};
  renderQueue.push(cmd);

  return nextFigureHex++;
}

void GraphicCore::_addTriangle(std::array<glm::vec2, 3> positions, glm::vec3 color,
                               uint32_t index) {
  std::lock_guard<std::mutex> verticesLock(verticesMutex);
  size_t startNum           = verticesCount;
  vertices[verticesCount++] = {positions[0], color};
  vertices[verticesCount++] = {positions[1], color};
  vertices[verticesCount++] = {positions[2], color};

  size_t startIndex        = indicesToDraw;
  indices[indicesToDraw++] = startNum;
  indices[indicesToDraw++] = startNum + 1;
  indices[indicesToDraw++] = startNum + 2;

  figures[index] = {.vertexOffset = startNum,
                    .vertexCount  = 3,
                    .firstIndex   = startIndex,
                    .indexCount   = 3,
                    .model        = glm::mat4(1.0f)};

  verticesChanged = true;
}

void GraphicCore::removeFigure(uint32_t index) {
  RenderCommand cmd;
  cmd.type = RemoveFig;
  cmd.data = RenderCommand::Remove{.index = index};
  renderQueue.push(cmd);
}

void GraphicCore::_removeFigure(uint32_t index) {
  std::lock_guard<std::mutex> verticesLock(verticesMutex);
  if (figures.count(index) == 0)
    return;

  FigureDesc fig = figures[index];
  figures.erase(index);
  size_t vertexCount = 3;
  if (fig.indexCount == 6)
    vertexCount = 4;
  for (size_t i = 0; i < fig.indexCount; ++i) {
    indices.erase(indices.begin() + fig.firstIndex);
  }
  for (size_t i = 0; i < vertexCount; ++i) {
    vertices.erase(vertices.begin() + fig.vertexOffset);
  }

  for (auto& i : indices) {
    if (i >= fig.vertexOffset + vertexCount) {
      i -= vertexCount;
    }
  }

  indicesToDraw -= fig.indexCount;
  verticesCount -= vertexCount;
  vertices.resize(MAX_VERTICES);
  indices.resize(MAX_INDICES);

  for (auto& [hex, desc] : figures) {
    if (desc.firstIndex > fig.firstIndex)
      desc.firstIndex -= fig.indexCount;
    if (desc.vertexOffset > fig.vertexOffset)
      desc.vertexOffset -= vertexCount;
  }

  verticesChanged = true;
}

void GraphicCore::setTransform(uint32_t index, const glm::mat4& transform) {
  RenderCommand cmd;
  cmd.type = SetTransform;
  cmd.data = RenderCommand::Transform{.index = index, .model = transform};
  renderQueue.push(cmd);
}

void GraphicCore::_setTranform(uint32_t index, glm::mat4 transform) {
  if (figures.count(index) == 0)
    return;
  figures[index].model = transform;
}

void GraphicCore::setCamera(glm::vec2 position, float zoom) {
  RenderCommand cmd;
  cmd.type = SetCamera;
  cmd.data = RenderCommand::Camera{.position = position, .zoom = zoom};
  renderQueue.push(cmd);
}

void GraphicCore::_setCamera(glm::vec2 position, float zoom) {
  camera.position = position;
  camera.zoom     = zoom;
}

TextureDescriptor GraphicCore::addTexture(const std::string& path) {
  int texWidth, texHeight, texChannels;
  stbi_uc* pixels = stbi_load(path.data(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);

  if (!pixels) {
    throw std::runtime_error("failed to load texture image!");
  }

  uint32_t arrayId;
  bool found = false;
  for (size_t i = 0; i < textureArrays.size(); ++i) {
    if (texWidth == textureArrays[i].extent.width && texHeight == textureArrays[i].extent.height) {
      found   = true;
      arrayId = i;
      break;
    }
  }
  if (!found) {
    VkExtent2D extent = {.width = texWidth, .height = texHeight};
    textureArrays.push_back(TextureArray{.extent = extent});
    arrayId = textureArrays.size() - 1;
  }
  textureArrays[arrayId].pixels.push_back(pixels);
  textureArrays[arrayId].layersCount += 1;
  TextureDescriptor desc = {.arrayId = arrayId, .layerId = textureArrays[arrayId].layersCount - 1};
  return desc;
}

void GraphicCore::addDefaultTexture() {
  stbi_uc* pixels = new stbi_uc[4]{255, 255, 255, 255};
  uint32_t arrayId;
  VkExtent2D extent = {.width = 1, .height = 1};
  textureArrays.push_back(TextureArray{.extent = extent});
  arrayId = textureArrays.size() - 1;

  textureArrays[arrayId].pixels.push_back(pixels);
  textureArrays[arrayId].layersCount += 1;
}

void GraphicCore::setTexture(uint32_t figureIndex, TextureDescriptor textureDescriptor,
                             const std::vector<glm::vec2>& texCoords) {
  RenderCommand cmd;
  cmd.type = SetTexture;
  cmd.data = RenderCommand::SetTexture{
    .index = figureIndex, .textureDesc = textureDescriptor, .texCoords = texCoords};
  renderQueue.push(cmd);
}

void GraphicCore::_setTexture(uint32_t figureIndex, TextureDescriptor textureDescriptor,
                              std::vector<glm::vec2> texCoords) {
  if (figures.count(figureIndex) == 0)
    return;
  figures[figureIndex].textureHandler =
    (textureDescriptor.arrayId << 16) | textureDescriptor.layerId;
  for (size_t i = 0; i < figures[figureIndex].vertexCount; ++i) {
    vertices[figures[figureIndex].vertexOffset + i].texCoord = texCoords[i];
  }
  verticesChanged = true;
}

void GraphicCore::startGraphicThread() {
  graphicThread = std::thread(&GraphicCore::run, this);
}

void GraphicCore::stopGraphicThread() {
  {
    std::lock_guard<std::mutex> stopLock(stopMutex);
    isStopped = true;
  }
  if (graphicThread.joinable())
    graphicThread.join();
}

VkVertexInputBindingDescription GraphicCore::Vertex::getBindingDescription() {
  VkVertexInputBindingDescription description{};
  description.binding   = 0;
  description.stride    = sizeof(Vertex);
  description.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

  return description;
}

std::array<VkVertexInputAttributeDescription, 3> GraphicCore::Vertex::getAttributeDescriptions() {
  std::array<VkVertexInputAttributeDescription, 3> attributeDescriptions{};

  attributeDescriptions[0].binding  = 0;
  attributeDescriptions[0].location = 0;
  attributeDescriptions[0].format   = VK_FORMAT_R32G32_SFLOAT;
  attributeDescriptions[0].offset   = offsetof(Vertex, pos);

  attributeDescriptions[1].binding  = 0;
  attributeDescriptions[1].location = 1;
  attributeDescriptions[1].format   = VK_FORMAT_R32G32B32_SFLOAT;
  attributeDescriptions[1].offset   = offsetof(Vertex, color);

  attributeDescriptions[2].binding  = 0;
  attributeDescriptions[2].location = 2;
  attributeDescriptions[2].format   = VK_FORMAT_R32G32_SFLOAT;
  attributeDescriptions[2].offset   = offsetof(Vertex, texCoord);

  return attributeDescriptions;
}

bool GraphicCore::QueueFamilyIndicies::isComplete() {
  return graphicFamily.has_value() && presentFamily.has_value();
}

std::vector<char> GraphicCore::readFile(const std::string& filename) {
  std::ifstream file(filename, std::ios::ate | std::ios::binary);

  if (!file.is_open()) {
    throw std::runtime_error("failed to open file!");
  }

  size_t fileSize = static_cast<size_t>(file.tellg());
  std::vector<char> buffer(fileSize);
  file.seekg(0);
  file.read(buffer.data(), fileSize);

  file.close();
  return buffer;
}

void GraphicCore::initWindow() {
  glfwSetWindowUserPointer(window, this);
  glfwSetFramebufferSizeCallback(window, framebufferSizeCallback);
}

void GraphicCore::initVulkan() {
  createInstance();
  setupDebugMessenger();
  createSurface();

  pickPhysicalDevice();
  createLogicalDevice();

  createSwapChain();

  createImageViews();
  createRenderPass();
  createDescriptorSetLayout();
  createGraphicsPipeline();

  createFrameBuffers();
  createCommandPool();

  createTextureImage();
  createTextureImageView();
  createTextureSampler();

  createVertexBuffer();
  createIndexBuffer();
  createUniformBuffers();
  createDescriptorPool();
  createDescriptorSets();

  createCommandBuffers();

  createSyncObjects();
}

void GraphicCore::createSurface() {
  if (glfwCreateWindowSurface(instance, window, nullptr, &surface) != VK_SUCCESS) {
    throw std::runtime_error("failed to create window surface!");
  }
}

void GraphicCore::mainLoop() {
  while (!glfwWindowShouldClose(window) && !isStopped) {
    pollRenderQueue();
    drawFrame();
  }

  vkDeviceWaitIdle(device);
}

void GraphicCore::framebufferSizeCallback(GLFWwindow* window, int width, int height) {
  auto app                = reinterpret_cast<GraphicCore*>(glfwGetWindowUserPointer(window));
  app->framebufferResized = true;
}

void GraphicCore::drawFrame() {
  vkWaitForFences(device, 1, &inFlightFences[currentFrame], VK_TRUE, UINT64_MAX);

  updateVertexBuffer();

  updateUniformBuffer(currentFrame);

  uint32_t imageIndex;
  VkResult result =
    vkAcquireNextImageKHR(device, swapChain, UINT64_MAX, imageAvailableSemaphores[currentFrame],
                          VK_NULL_HANDLE, &imageIndex);

  if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || framebufferResized) {
    framebufferResized = false;
    recreateSwapChain();
    return;
  } else if (result != VK_SUCCESS) {
    throw std::runtime_error("failed to acquire swapchain image");
  }

  if (imagesInFlight[imageIndex] != VK_NULL_HANDLE) {
    vkWaitForFences(device, 1, &imagesInFlight[imageIndex], VK_TRUE, UINT64_MAX);
  }
  imagesInFlight[imageIndex] = inFlightFences[currentFrame];

  vkResetCommandBuffer(commandBuffers[currentFrame], 0);
  recordCommandBuffer(commandBuffers[currentFrame], imageIndex);

  VkSubmitInfo submitInfo{};
  submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

  VkSemaphore waitSemaphores[]      = {imageAvailableSemaphores[currentFrame]};
  VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
  submitInfo.waitSemaphoreCount     = 1;
  submitInfo.pWaitSemaphores        = waitSemaphores;
  submitInfo.pWaitDstStageMask      = waitStages;

  submitInfo.commandBufferCount = 1;
  submitInfo.pCommandBuffers    = &commandBuffers[currentFrame];

  VkSemaphore signalSemaphores[]  = {renderFinishedSemaphores[currentFrame]};
  submitInfo.signalSemaphoreCount = 1;
  submitInfo.pSignalSemaphores    = signalSemaphores;

  vkResetFences(device, 1, &inFlightFences[currentFrame]);

  if (vkQueueSubmit(graphicsQueue, 1, &submitInfo, inFlightFences[currentFrame]) != VK_SUCCESS) {
    throw std::runtime_error("failed to submit draw command buffer");
  }

  VkPresentInfoKHR presentInfo{};
  presentInfo.sType              = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
  presentInfo.waitSemaphoreCount = 1;
  presentInfo.pWaitSemaphores    = signalSemaphores;
  VkSwapchainKHR swapChains[]    = {swapChain};
  presentInfo.swapchainCount     = 1;
  presentInfo.pSwapchains        = swapChains;
  presentInfo.pImageIndices      = &imageIndex;

  vkQueuePresentKHR(presentQueue, &presentInfo);

  currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}

void GraphicCore::pollRenderQueue() {
  RenderCommand cmd;
  while (renderQueue.pop(cmd)) {
    switch (cmd.type) {
      case AddRect: {
        auto& data = std::get<RenderCommand::AddRect>(cmd.data);
        _addRectangle(data.position, data.width, data.height, data.color, data.index);
        break;
      }
      case AddTriangle: {
        auto& data = std::get<RenderCommand::AddTri>(cmd.data);
        _addTriangle(data.positions, data.color, data.index);
        break;
      }
      case RemoveFig: {
        auto& data = std::get<RenderCommand::Remove>(cmd.data);
        _removeFigure(data.index);
        break;
      }
      case SetTransform: {
        auto& data = std::get<RenderCommand::Transform>(cmd.data);
        _setTranform(data.index, data.model);
        break;
      }
      case SetCamera: {
        auto& data = std::get<RenderCommand::Camera>(cmd.data);
        _setCamera(data.position, data.zoom);
        break;
      }
      case SetTexture: {
        auto& data = std::get<RenderCommand::SetTexture>(cmd.data);
        _setTexture(data.index, data.textureDesc, data.texCoords);
        break;
      }
      default:
        break;
    }
  }
}

void GraphicCore::cleanup() {
  for (size_t i = 0; i != imageAvailableSemaphores.size(); i++) {
    vkDestroySemaphore(device, imageAvailableSemaphores[i], nullptr);
    vkDestroySemaphore(device, renderFinishedSemaphores[i], nullptr);
    vkDestroyFence(device, inFlightFences[i], nullptr);
  }

  cleanupSwapChain();
  vkDestroySwapchainKHR(device, swapChain, nullptr);

  for (auto& textureArray : textureArrays) {
    vkDestroySampler(device, textureArray.sampler, nullptr);
    vkDestroyImageView(device, textureArray.imageView, nullptr);
    vkDestroyImage(device, textureArray.image, nullptr);
    vkFreeMemory(device, textureArray.imageMemory, nullptr);
  }

  vkDestroyBuffer(device, vertexBuffer, nullptr);
  vkFreeMemory(device, vertexBufferMemory, nullptr);
  vkDestroyBuffer(device, stagingVertexBuffer, nullptr);
  vkFreeMemory(device, stagingVertexBufferMemory, nullptr);

  for (int i = 0; i != MAX_FRAMES_IN_FLIGHT; ++i) {
    vkDestroyBuffer(device, uniformBuffers[i], nullptr);
    vkFreeMemory(device, uniformBuffersMemory[i], nullptr);
  }

  vkDestroyDescriptorPool(device, descriptorPool, nullptr);

  vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);

  vkDestroyBuffer(device, indexBuffer, nullptr);
  vkFreeMemory(device, indexBufferMemory, nullptr);
  vkDestroyBuffer(device, stagingIndexBuffer, nullptr);
  vkFreeMemory(device, stagingIndexBufferMemory, nullptr);

  vkDestroyCommandPool(device, commandPool, nullptr);

  vkDestroyDevice(device, nullptr);

  if (enableValidationLayers)
    DestroyDebugUtilsMessengerEXT(instance, debugMessenger, nullptr);

  vkDestroySurfaceKHR(instance, surface, nullptr);
  vkDestroyInstance(instance, nullptr);
}

bool GraphicCore::checkExtensionsSupport(const uint32_t extCount, const char** extToCheck) {
  for (uint32_t i = 0; i != extCount; ++i) {
    bool supported = false;
    for (auto& ext : instanceExtensions) {
      if (strcmp(*(extToCheck + i), ext.extensionName) == 0) {
        supported = true;
      }
    }
    if (!supported)
      return false;
  }

  return true;
}

void GraphicCore::createLogicalDevice() {
  QueueFamilyIndicies indicies = findQueueFamilies(physicalDevice);
  float queuePriority          = 1.0f;
  std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
  std::set<uint32_t> uniqueQueueFamilies = {indicies.graphicFamily.value(),
                                            indicies.presentFamily.value()};
  for (uint32_t queueFamily : uniqueQueueFamilies) {
    VkDeviceQueueCreateInfo queueCreateInfo{};
    queueCreateInfo.sType            = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queueCreateInfo.queueFamilyIndex = indicies.graphicFamily.value();
    queueCreateInfo.queueCount       = 1;
    queueCreateInfo.pQueuePriorities = &queuePriority;
    queueCreateInfos.push_back(queueCreateInfo);
  }

  VkPhysicalDeviceFeatures deviceFeatures{};
  deviceFeatures.samplerAnisotropy = VK_TRUE;

  VkDeviceCreateInfo createInfo{};
  createInfo.sType                   = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
  createInfo.pQueueCreateInfos       = queueCreateInfos.data();
  createInfo.queueCreateInfoCount    = static_cast<uint32_t>(queueCreateInfos.size());
  createInfo.pEnabledFeatures        = &deviceFeatures;
  createInfo.enabledExtensionCount   = static_cast<uint32_t>(deviceExtensions.size());
  createInfo.ppEnabledExtensionNames = deviceExtensions.data();

  if (enableValidationLayers) {
    createInfo.enabledLayerCount   = static_cast<uint32_t>(validationLayers.size());
    createInfo.ppEnabledLayerNames = validationLayers.data();
  } else {
    createInfo.enabledLayerCount = 0;
  }

  if (vkCreateDevice(physicalDevice, &createInfo, nullptr, &device) != VK_SUCCESS) {
    throw std::runtime_error("failed to create logical device!");
  }
  vkGetDeviceQueue(device, indicies.graphicFamily.value(), 0, &graphicsQueue);
  vkGetDeviceQueue(device, indicies.presentFamily.value(), 0, &presentQueue);
}

void GraphicCore::createSwapChain() {
  SwapChainSupportDetails swapChainSupport = querySwapChainSupport(physicalDevice);

  VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);
  VkPresentModeKHR presentMode     = chooseSwapPresentMode(swapChainSupport.presentModes);
  VkExtent2D extent                = chooseSwapExtent(swapChainSupport.capabilities);

  uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
  if (swapChainSupport.capabilities.maxImageCount > 0 &&
      imageCount > swapChainSupport.capabilities.maxImageCount) {
    imageCount = swapChainSupport.capabilities.maxImageCount;
  }

  VkSwapchainCreateInfoKHR createInfo{};
  createInfo.sType            = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
  createInfo.surface          = surface;
  createInfo.minImageCount    = imageCount;
  createInfo.imageFormat      = surfaceFormat.format;
  createInfo.imageColorSpace  = surfaceFormat.colorSpace;
  createInfo.imageExtent      = extent;
  createInfo.imageArrayLayers = 1;
  createInfo.imageUsage       = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

  QueueFamilyIndicies indicies   = findQueueFamilies(physicalDevice);
  uint32_t queueFamilyIndicies[] = {indicies.graphicFamily.value(), indicies.presentFamily.value()};

  if (indicies.graphicFamily != indicies.presentFamily) {
    createInfo.imageSharingMode      = VK_SHARING_MODE_CONCURRENT;
    createInfo.queueFamilyIndexCount = 2;
    createInfo.pQueueFamilyIndices   = queueFamilyIndicies;
  } else {
    createInfo.imageSharingMode      = VK_SHARING_MODE_EXCLUSIVE;
    createInfo.queueFamilyIndexCount = 0;
    createInfo.pQueueFamilyIndices   = nullptr;
  }

  createInfo.preTransform   = swapChainSupport.capabilities.currentTransform;
  createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;

  createInfo.presentMode  = presentMode;
  createInfo.clipped      = VK_TRUE;
  createInfo.oldSwapchain = swapChain;

  auto oldSwapChain = swapChain;

  if (vkCreateSwapchainKHR(device, &createInfo, nullptr, &swapChain) != VK_SUCCESS) {
    throw std::runtime_error("failed to create the swapchain!");
  }

  if (oldSwapChain != VK_NULL_HANDLE) {
    vkDestroySwapchainKHR(device, oldSwapChain, nullptr);
  }

  uint32_t swapChainImagesCount;
  vkGetSwapchainImagesKHR(device, swapChain, &swapChainImagesCount, nullptr);
  swapChainImages.resize(swapChainImagesCount);
  vkGetSwapchainImagesKHR(device, swapChain, &swapChainImagesCount, swapChainImages.data());
  swapChainImageFormat = surfaceFormat.format;
  swapChainExtent      = extent;
}

void GraphicCore::cleanupSwapChain() {
  for (auto framebuffer : swapChainFramebuffers) {
    vkDestroyFramebuffer(device, framebuffer, nullptr);
  }

  vkFreeCommandBuffers(device, commandPool, static_cast<uint32_t>(commandBuffers.size()),
                       commandBuffers.data());

  vkDestroyPipeline(device, graphicsPipeline, nullptr);
  vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
  vkDestroyRenderPass(device, renderPass, nullptr);
  for (auto imageView : swapChainImageViews) {
    vkDestroyImageView(device, imageView, nullptr);
  }
}

void GraphicCore::recreateSwapChain() {
  int width = 0, height = 0;
  glfwGetFramebufferSize(window, &width, &height);
  while (width == 0 || height == 0) {
    glfwGetFramebufferSize(window, &width, &height);
    glfwWaitEvents();
  }

  vkDeviceWaitIdle(device);

  cleanupSwapChain();

  createSwapChain();
  createImageViews();
  createRenderPass();
  createGraphicsPipeline();
  createFrameBuffers();
  createCommandBuffers();
}

void GraphicCore::createImageViews() {
  swapChainImageViews.resize(swapChainImages.size());
  for (size_t i = 0; i != swapChainImages.size(); ++i) {
    swapChainImageViews[i] =
      createImageView(swapChainImages[i], swapChainImageFormat, 1, VK_IMAGE_VIEW_TYPE_2D);
  }
}

void GraphicCore::createRenderPass() {
  VkAttachmentDescription colorAttachment{};
  colorAttachment.format         = swapChainImageFormat;
  colorAttachment.samples        = VK_SAMPLE_COUNT_1_BIT;
  colorAttachment.loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR;
  colorAttachment.storeOp        = VK_ATTACHMENT_STORE_OP_STORE;
  colorAttachment.stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
  colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  colorAttachment.initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;
  colorAttachment.finalLayout    = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

  VkAttachmentReference colorAttachmentRef{};
  colorAttachmentRef.attachment = 0;
  colorAttachmentRef.layout     = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

  VkSubpassDescription subpass{};
  subpass.pipelineBindPoint    = VK_PIPELINE_BIND_POINT_GRAPHICS;
  subpass.colorAttachmentCount = 1;
  subpass.pColorAttachments    = &colorAttachmentRef;

  VkRenderPassCreateInfo renderPassInfo{};
  renderPassInfo.sType           = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
  renderPassInfo.attachmentCount = 1;
  renderPassInfo.pAttachments    = &colorAttachment;
  renderPassInfo.subpassCount    = 1;
  renderPassInfo.pSubpasses      = &subpass;

  VkSubpassDependency dependency{};
  dependency.srcSubpass    = VK_SUBPASS_EXTERNAL;
  dependency.dstSubpass    = 0;
  dependency.srcStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  dependency.srcAccessMask = 0;
  dependency.dstStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

  renderPassInfo.dependencyCount = 1;
  renderPassInfo.pDependencies   = &dependency;

  if (vkCreateRenderPass(device, &renderPassInfo, nullptr, &renderPass) != VK_SUCCESS) {
    throw std::runtime_error("failed to create render pass!");
  }
}

void GraphicCore::createDescriptorSetLayout() {
  VkDescriptorSetLayoutBinding uboLayoutBinding{};
  uboLayoutBinding.binding         = 0;
  uboLayoutBinding.descriptorCount = 1;
  uboLayoutBinding.descriptorType  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  uboLayoutBinding.stageFlags      = VK_SHADER_STAGE_VERTEX_BIT;

  VkDescriptorSetLayoutBinding samplerLayoutBinding{};
  samplerLayoutBinding.binding            = 1;
  samplerLayoutBinding.descriptorCount    = MAX_IMAGE_ARRAYS;
  samplerLayoutBinding.descriptorType     = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  samplerLayoutBinding.pImmutableSamplers = nullptr;
  samplerLayoutBinding.stageFlags         = VK_SHADER_STAGE_FRAGMENT_BIT;

  std::array<VkDescriptorSetLayoutBinding, 2> bindings = {uboLayoutBinding, samplerLayoutBinding};

  VkDescriptorSetLayoutCreateInfo layoutInfo{};
  layoutInfo.sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
  layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
  layoutInfo.pBindings    = bindings.data();

  if (vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &descriptorSetLayout) !=
      VK_SUCCESS) {
    throw std::runtime_error("failed to create descriptor set layout!");
  }
}

void GraphicCore::createGraphicsPipeline() {
  auto vertShaderCode             = readFile("../shaders/vert.spv");
  auto fragShaderCode             = readFile("../shaders/frag.spv");
  VkShaderModule vertShaderModule = createShaderModule(vertShaderCode);
  VkShaderModule fragShaderModule = createShaderModule(fragShaderCode);

  VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
  vertShaderStageInfo.sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  vertShaderStageInfo.stage  = VK_SHADER_STAGE_VERTEX_BIT;
  vertShaderStageInfo.module = vertShaderModule;
  vertShaderStageInfo.pName  = "main";

  VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
  fragShaderStageInfo.sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  fragShaderStageInfo.stage  = VK_SHADER_STAGE_FRAGMENT_BIT;
  fragShaderStageInfo.module = fragShaderModule;
  fragShaderStageInfo.pName  = "main";

  VkPipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo, fragShaderStageInfo};

  VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
  vertexInputInfo.sType      = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
  auto bindingDescription    = Vertex::getBindingDescription();
  auto attributeDescriptions = Vertex::getAttributeDescriptions();
  vertexInputInfo.vertexBindingDescriptionCount = 1;
  vertexInputInfo.pVertexBindingDescriptions    = &bindingDescription;
  vertexInputInfo.vertexAttributeDescriptionCount =
    static_cast<uint32_t>(attributeDescriptions.size());
  vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

  VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
  inputAssembly.sType    = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
  inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
  inputAssembly.primitiveRestartEnable = VK_FALSE;

  VkViewport viewport{};
  viewport.x        = 0.0f;
  viewport.y        = 0.0f;
  viewport.height   = swapChainExtent.height;
  viewport.width    = swapChainExtent.width;
  viewport.minDepth = 0.0f;
  viewport.maxDepth = 1.0f;

  VkRect2D scissor{};
  scissor.offset = {0, 0};
  scissor.extent = swapChainExtent;

  VkPipelineViewportStateCreateInfo viewportState{};
  viewportState.sType         = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
  viewportState.viewportCount = 1;
  viewportState.pViewports    = &viewport;
  viewportState.scissorCount  = 1;
  viewportState.pScissors     = &scissor;

  VkPipelineRasterizationStateCreateInfo rasterizer{};
  rasterizer.sType                   = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
  rasterizer.depthClampEnable        = VK_FALSE;
  rasterizer.rasterizerDiscardEnable = VK_FALSE;
  rasterizer.polygonMode             = VK_POLYGON_MODE_FILL;
  rasterizer.lineWidth               = 1.0f;
  rasterizer.cullMode                = VK_CULL_MODE_BACK_BIT;
  rasterizer.frontFace               = VK_FRONT_FACE_CLOCKWISE;
  rasterizer.depthBiasEnable         = VK_FALSE;

  VkPipelineMultisampleStateCreateInfo multisampling{};
  multisampling.sType                = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
  multisampling.sampleShadingEnable  = VK_FALSE;
  multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

  VkPipelineColorBlendAttachmentState colorBlendAttachment{};
  colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                                        VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
  colorBlendAttachment.blendEnable = VK_FALSE;

  VkPipelineColorBlendStateCreateInfo colorBlending{};
  colorBlending.sType           = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
  colorBlending.logicOpEnable   = VK_FALSE;
  colorBlending.attachmentCount = 1;
  colorBlending.pAttachments    = &colorBlendAttachment;

  VkDynamicState dynamicStates[] = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_LINE_WIDTH};

  VkPipelineDynamicStateCreateInfo dynamicState{};
  dynamicState.sType             = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
  dynamicState.dynamicStateCount = 2;
  dynamicState.pDynamicStates    = dynamicStates;

  VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
  pipelineLayoutInfo.sType          = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
  pipelineLayoutInfo.setLayoutCount = 1;
  pipelineLayoutInfo.pSetLayouts    = &descriptorSetLayout;

  VkPushConstantRange pcRange{};
  pcRange.offset     = 0;
  pcRange.size       = sizeof(PushConstant);
  pcRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;

  pipelineLayoutInfo.pushConstantRangeCount = 1;
  pipelineLayoutInfo.pPushConstantRanges    = &pcRange;

  if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS) {
    throw std::runtime_error("failed to create pipeline layout!");
  }

  VkGraphicsPipelineCreateInfo pipelineInfo{};
  pipelineInfo.sType      = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
  pipelineInfo.stageCount = 2;
  pipelineInfo.pStages    = shaderStages;

  pipelineInfo.pVertexInputState   = &vertexInputInfo;
  pipelineInfo.pInputAssemblyState = &inputAssembly;
  pipelineInfo.pViewportState      = &viewportState;
  pipelineInfo.pRasterizationState = &rasterizer;
  pipelineInfo.pMultisampleState   = &multisampling;
  pipelineInfo.pDepthStencilState  = nullptr;
  pipelineInfo.pColorBlendState    = &colorBlending;
  pipelineInfo.pDynamicState       = nullptr;  //&dynamicState;

  pipelineInfo.layout     = pipelineLayout;
  pipelineInfo.renderPass = renderPass;
  pipelineInfo.subpass    = 0;

  pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
  pipelineInfo.basePipelineIndex  = -1;

  if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr,
                                &graphicsPipeline) != VK_SUCCESS) {
    throw std::runtime_error("failed to create graphics pipeline!");
  }

  vkDestroyShaderModule(device, vertShaderModule, nullptr);
  vkDestroyShaderModule(device, fragShaderModule, nullptr);
}

void GraphicCore::createFrameBuffers() {
  swapChainFramebuffers.resize(swapChainImageViews.size());
  for (size_t i = 0; i < swapChainImageViews.size(); ++i) {
    VkImageView attachments[] = {swapChainImageViews[i]};

    VkFramebufferCreateInfo framebufferInfo{};
    framebufferInfo.sType           = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    framebufferInfo.renderPass      = renderPass;
    framebufferInfo.attachmentCount = 1;
    framebufferInfo.pAttachments    = attachments;
    framebufferInfo.width           = swapChainExtent.width;
    framebufferInfo.height          = swapChainExtent.height;
    framebufferInfo.layers          = 1;

    if (vkCreateFramebuffer(device, &framebufferInfo, nullptr, &swapChainFramebuffers[i]) !=
        VK_SUCCESS) {
      throw std::runtime_error("failed to create framebuffers!");
    }
  }
}

void GraphicCore::createCommandPool() {
  QueueFamilyIndicies queueFamilyIndicies = findQueueFamilies(physicalDevice);

  VkCommandPoolCreateInfo poolInfo{};
  poolInfo.sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
  poolInfo.queueFamilyIndex = queueFamilyIndicies.graphicFamily.value();
  poolInfo.flags            = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

  if (vkCreateCommandPool(device, &poolInfo, nullptr, &commandPool) != VK_SUCCESS) {
    throw std::runtime_error("failed to create command pool!");
  }
}

void GraphicCore::createBuffer(VkDeviceSize size, VkBufferUsageFlags usage,
                               VkMemoryPropertyFlags properties, VkBuffer& buffer,
                               VkDeviceMemory& bufferMemory) {
  VkBufferCreateInfo bufferInfo{};
  bufferInfo.sType       = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  bufferInfo.size        = size;
  bufferInfo.usage       = usage;
  bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

  if (vkCreateBuffer(device, &bufferInfo, nullptr, &buffer) != VK_SUCCESS) {
    throw std::runtime_error("failed to create vertex buffer!");
  }

  VkMemoryRequirements memRequirements;
  vkGetBufferMemoryRequirements(device, buffer, &memRequirements);

  VkMemoryAllocateInfo allocInfo{};
  allocInfo.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  allocInfo.allocationSize  = memRequirements.size;
  allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties);

  if (vkAllocateMemory(device, &allocInfo, nullptr, &bufferMemory) != VK_SUCCESS) {
    throw std::runtime_error("failed to allocate vertex buffer memory!");
  }

  vkBindBufferMemory(device, buffer, bufferMemory, 0);
}

void GraphicCore::createVertexBuffer() {
  std::lock_guard<std::mutex> verticesLock(verticesMutex);

  VkDeviceSize bufferSize = sizeof(Vertex) * vertices.size();
  VkMemoryPropertyFlags propertyFlags =
    VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;

  createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, propertyFlags, stagingVertexBuffer,
               stagingVertexBufferMemory);

  void* data;
  vkMapMemory(device, stagingVertexBufferMemory, 0, sizeof(Vertex) * vertices.size(), 0, &data);
  memcpy(data, vertices.data(), sizeof(Vertex) * vertices.size());
  vkUnmapMemory(device, stagingVertexBufferMemory);

  createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
               VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, vertexBuffer, vertexBufferMemory);

  copyBuffer(stagingVertexBuffer, vertexBuffer, bufferSize);
}

void GraphicCore::createIndexBuffer() {
  VkDeviceSize bufferSize = sizeof(uint16_t) * indices.size();
  VkMemoryPropertyFlags propertyFlags =
    VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;

  createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, propertyFlags, stagingIndexBuffer,
               stagingIndexBufferMemory);

  void* data;
  vkMapMemory(device, stagingIndexBufferMemory, 0, bufferSize, 0, &data);
  memcpy(data, indices.data(), bufferSize);
  vkUnmapMemory(device, stagingIndexBufferMemory);

  createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
               VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, indexBuffer, indexBufferMemory);

  copyBuffer(stagingIndexBuffer, indexBuffer, bufferSize);
}

void GraphicCore::createImage(uint32_t width, uint32_t height, VkFormat format,
                              VkImageTiling tiling, VkImageUsageFlags usage,
                              VkMemoryPropertyFlags properties, VkImage& image,
                              VkDeviceMemory& imageMemory, uint32_t arrayLayers) {
  VkImageCreateInfo imageInfo{};
  imageInfo.sType         = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
  imageInfo.imageType     = VK_IMAGE_TYPE_2D;
  imageInfo.extent.width  = width;
  imageInfo.extent.height = height;
  imageInfo.arrayLayers   = arrayLayers;
  imageInfo.extent.depth  = 1;
  imageInfo.mipLevels     = 1;
  imageInfo.format        = format;
  imageInfo.tiling        = tiling;
  imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  imageInfo.usage         = usage;
  imageInfo.sharingMode   = VK_SHARING_MODE_EXCLUSIVE;
  imageInfo.samples       = VK_SAMPLE_COUNT_1_BIT;

  if (vkCreateImage(device, &imageInfo, nullptr, &image) != VK_SUCCESS) {
    throw std::runtime_error("failed to create image!");
  }

  VkMemoryRequirements memRequirements;
  vkGetImageMemoryRequirements(device, image, &memRequirements);

  VkMemoryAllocateInfo allocInfo{};
  allocInfo.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties);
  allocInfo.allocationSize  = memRequirements.size;

  if (vkAllocateMemory(device, &allocInfo, nullptr, &imageMemory) != VK_SUCCESS) {
    throw std::runtime_error("failed to allocate texture memory!");
  }

  vkBindImageMemory(device, image, imageMemory, 0);
}

void GraphicCore::createTextureImage() {
  for (auto& textureArray : textureArrays) {
    uint64_t layerSize     = textureArray.extent.width * textureArray.extent.height * 4;
    VkDeviceSize imageSize = layerSize * textureArray.layersCount;
    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    createBuffer(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                 stagingBuffer, stagingBufferMemory);

    void* data;
    vkMapMemory(device, stagingBufferMemory, 0, imageSize, 0, &data);
    for (int i = 0; i != textureArray.layersCount; ++i) {
      memcpy(data + i * layerSize, textureArray.pixels[i], static_cast<size_t>(layerSize));
      stbi_image_free(textureArray.pixels[i]);
    }
    vkUnmapMemory(device, stagingBufferMemory);

    createImage(textureArray.extent.width, textureArray.extent.height, VK_FORMAT_R8G8B8A8_SRGB,
                VK_IMAGE_TILING_OPTIMAL,
                VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, textureArray.image, textureArray.imageMemory,
                textureArray.layersCount);

    transitionImageLayout(textureArray.image, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED,
                          VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, textureArray.layersCount);
    copyBufferToImage(stagingBuffer, textureArray.image,
                      static_cast<uint32_t>(textureArray.extent.width),
                      static_cast<uint32_t>(textureArray.extent.height), textureArray.layersCount);
    transitionImageLayout(textureArray.image, VK_FORMAT_R8G8B8A8_SRGB,
                          VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                          VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, textureArray.layersCount);

    vkDestroyBuffer(device, stagingBuffer, nullptr);
    vkFreeMemory(device, stagingBufferMemory, nullptr);
  }
}

void GraphicCore::createTextureImageView() {
  for (auto& textureArray : textureArrays) {
    textureArray.imageView = createImageView(textureArray.image, VK_FORMAT_R8G8B8A8_SRGB,
                                             textureArray.layersCount, VK_IMAGE_VIEW_TYPE_2D_ARRAY);
  }
}

VkImageView GraphicCore::createImageView(VkImage image, VkFormat format, uint32_t layerCount,
                                         VkImageViewType viewType) {
  VkImageViewCreateInfo viewInfo{};
  viewInfo.sType                           = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
  viewInfo.image                           = image;
  viewInfo.viewType                        = viewType;
  viewInfo.format                          = format;
  viewInfo.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
  viewInfo.subresourceRange.baseMipLevel   = 0;
  viewInfo.subresourceRange.levelCount     = 1;
  viewInfo.subresourceRange.baseArrayLayer = 0;
  viewInfo.subresourceRange.layerCount     = layerCount;

  VkImageView imageView;
  if (vkCreateImageView(device, &viewInfo, nullptr, &imageView) != VK_SUCCESS) {
    throw std::runtime_error("failed to create image view!");
  }

  return imageView;
}

void GraphicCore::createTextureSampler() {
  for (auto& textureArray : textureArrays) {
    VkSamplerCreateInfo samplerInfo{};
    samplerInfo.sType            = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter        = VK_FILTER_LINEAR;
    samplerInfo.minFilter        = VK_FILTER_LINEAR;
    samplerInfo.addressModeU     = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeV     = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeW     = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.anisotropyEnable = VK_TRUE;
    VkPhysicalDeviceProperties properties{};
    vkGetPhysicalDeviceProperties(physicalDevice, &properties);
    samplerInfo.maxAnisotropy           = properties.limits.maxSamplerAnisotropy;
    samplerInfo.borderColor             = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    samplerInfo.unnormalizedCoordinates = VK_FALSE;
    samplerInfo.compareEnable           = VK_FALSE;
    samplerInfo.compareOp               = VK_COMPARE_OP_ALWAYS;
    samplerInfo.mipmapMode              = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    samplerInfo.mipLodBias              = 0.0f;
    samplerInfo.minLod                  = 0.0f;
    samplerInfo.maxLod                  = 0.0f;

    if (vkCreateSampler(device, &samplerInfo, nullptr, &textureArray.sampler) != VK_SUCCESS) {
      throw std::runtime_error("failed to create texture sampler!");
    }
  }
}

VkCommandBuffer GraphicCore::beginSingleTimeCommands() {
  VkCommandBufferAllocateInfo allocInfo{};
  allocInfo.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  allocInfo.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  allocInfo.commandPool        = commandPool;
  allocInfo.commandBufferCount = 1;

  VkCommandBuffer commandBuffer;
  vkAllocateCommandBuffers(device, &allocInfo, &commandBuffer);

  VkCommandBufferBeginInfo beginInfo{};
  beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

  vkBeginCommandBuffer(commandBuffer, &beginInfo);
  return commandBuffer;
}

void GraphicCore::endSingleTimeCommands(VkCommandBuffer commandBuffer) {
  vkEndCommandBuffer(commandBuffer);

  VkSubmitInfo submitInfo{};
  submitInfo.sType              = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  submitInfo.commandBufferCount = 1;
  submitInfo.pCommandBuffers    = &commandBuffer;

  vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
  vkQueueWaitIdle(graphicsQueue);
  vkFreeCommandBuffers(device, commandPool, 1, &commandBuffer);
}

void GraphicCore::transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout,
                                        VkImageLayout newLayout, uint32_t layerCount) {
  VkCommandBuffer commandBuffer = beginSingleTimeCommands();

  VkImageMemoryBarrier barrier{};
  barrier.sType                           = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
  barrier.oldLayout                       = oldLayout;
  barrier.newLayout                       = newLayout;
  barrier.srcQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
  barrier.dstQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
  barrier.image                           = image;
  barrier.image                           = image;
  barrier.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
  barrier.subresourceRange.baseMipLevel   = 0;
  barrier.subresourceRange.levelCount     = 1;
  barrier.subresourceRange.baseArrayLayer = 0;
  barrier.subresourceRange.layerCount     = layerCount;

  VkPipelineStageFlags sourceStage;
  VkPipelineStageFlags destinationStage;

  if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
    barrier.srcAccessMask = 0;
    barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

    sourceStage      = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
    destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
  } else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL &&
             newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

    sourceStage      = VK_PIPELINE_STAGE_TRANSFER_BIT;
    destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
  } else {
    throw std::invalid_argument("unsupported layout transition!");
  }

  vkCmdPipelineBarrier(commandBuffer, sourceStage, destinationStage, 0, 0, nullptr, 0, nullptr, 1,
                       &barrier);

  endSingleTimeCommands(commandBuffer);
}

void GraphicCore::copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height,
                                    uint32_t layerCount) {
  VkCommandBuffer commandBuffer = beginSingleTimeCommands();

  VkBufferImageCopy region{};
  region.bufferOffset                    = 0;
  region.bufferRowLength                 = 0;
  region.bufferImageHeight               = 0;
  region.imageSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
  region.imageSubresource.layerCount     = layerCount;
  region.imageSubresource.mipLevel       = 0;
  region.imageSubresource.baseArrayLayer = 0;
  region.imageOffset                     = {0, 0, 0};
  region.imageExtent                     = {width, height, 1};

  vkCmdCopyBufferToImage(commandBuffer, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1,
                         &region);

  endSingleTimeCommands(commandBuffer);
}

void GraphicCore::createUniformBuffers() {
  VkDeviceSize bufferSize = sizeof(UniformBufferObject);

  uniformBuffers.resize(MAX_FRAMES_IN_FLIGHT);
  uniformBuffersMemory.resize(MAX_FRAMES_IN_FLIGHT);
  uniformBuffersMapped.resize(MAX_FRAMES_IN_FLIGHT);

  for (int i = 0; i != MAX_FRAMES_IN_FLIGHT; ++i) {
    createBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                 VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
                 uniformBuffers[i], uniformBuffersMemory[i]);
    vkMapMemory(device, uniformBuffersMemory[i], 0, bufferSize, 0, &uniformBuffersMapped[i]);
  }
}

void GraphicCore::updateUniformBuffer(uint32_t currentImage) {
  UniformBufferObject ubo{};
  ubo.view = glm::translate(glm::mat4(1.0f), glm::vec3(-camera.position, 0)) *
             glm::scale(glm::mat4(1.0f), glm::vec3(camera.zoom, camera.zoom, 1));
  ubo.proj = glm::ortho(-(float)swapChainExtent.width / 2.f, (float)swapChainExtent.width / 2.f,
                        -(float)swapChainExtent.height / 2.f, (float)swapChainExtent.height / 2.f);

  memcpy(uniformBuffersMapped[currentFrame], &ubo, sizeof(ubo));
}

void GraphicCore::updateVertexBuffer() {
  if (verticesChanged) {
    std::lock_guard<std::mutex> verticesLock(verticesMutex);
    void* vertexData;
    vkMapMemory(device, stagingVertexBufferMemory, 0, sizeof(Vertex) * vertices.size(), 0,
                &vertexData);
    memcpy(vertexData, vertices.data(), sizeof(Vertex) * vertices.size());
    vkUnmapMemory(device, stagingVertexBufferMemory);

    copyBuffer(stagingVertexBuffer, vertexBuffer, sizeof(Vertex) * vertices.size());

    void* indexData;
    vkMapMemory(device, stagingIndexBufferMemory, 0, sizeof(uint16_t) * indices.size(), 0,
                &indexData);
    memcpy(indexData, indices.data(), sizeof(uint16_t) * indices.size());
    vkUnmapMemory(device, stagingIndexBufferMemory);

    copyBuffer(stagingIndexBuffer, indexBuffer, sizeof(uint16_t) * indices.size());
    verticesChanged = false;
  }
}

void GraphicCore::createDescriptorPool() {
  std::array<VkDescriptorPoolSize, 2> poolSizes{};
  poolSizes[0].type            = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  poolSizes[0].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
  poolSizes[1].type            = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  poolSizes[1].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT * MAX_IMAGE_ARRAYS);

  VkDescriptorPoolCreateInfo poolInfo{};
  poolInfo.sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
  poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
  poolInfo.pPoolSizes    = poolSizes.data();
  poolInfo.maxSets       = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT * MAX_IMAGE_ARRAYS);

  if (vkCreateDescriptorPool(device, &poolInfo, nullptr, &descriptorPool) != VK_SUCCESS) {
    throw std::runtime_error("failed to create descriptor pool!");
  }
}

void GraphicCore::createDescriptorSets() {
  std::vector<VkDescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, descriptorSetLayout);
  VkDescriptorSetAllocateInfo allocInfo{};
  allocInfo.sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
  allocInfo.descriptorSetCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
  allocInfo.descriptorPool     = descriptorPool;
  allocInfo.pSetLayouts        = layouts.data();

  descriptorSets.resize(MAX_FRAMES_IN_FLIGHT);
  if (vkAllocateDescriptorSets(device, &allocInfo, descriptorSets.data()) != VK_SUCCESS) {
    throw std::runtime_error("failed to allocate descriptor sets!");
  }

  for (size_t i = 0; i != MAX_FRAMES_IN_FLIGHT; ++i) {
    VkDescriptorBufferInfo bufferInfo{};
    bufferInfo.buffer = uniformBuffers[i];
    bufferInfo.offset = 0;
    bufferInfo.range  = sizeof(UniformBufferObject);

    std::vector<VkDescriptorImageInfo> imageInfos(MAX_IMAGE_ARRAYS);
    for (size_t i = 0; i < textureArrays.size(); ++i) {
      imageInfos[i].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
      imageInfos[i].imageView   = textureArrays[i].imageView;
      imageInfos[i].sampler     = textureArrays[i].sampler;
      std::cout << "extent: " << textureArrays[i].extent.width << '\n';
    }
    for (size_t i = textureArrays.size(); i < MAX_IMAGE_ARRAYS; ++i) {
      imageInfos[i] = imageInfos[0];
    }
    std::array<VkWriteDescriptorSet, 2> descriptorWrites{};
    descriptorWrites[0].sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrites[0].descriptorCount = 1;
    descriptorWrites[0].dstSet          = descriptorSets[i];
    descriptorWrites[0].dstBinding      = 0;
    descriptorWrites[0].dstArrayElement = 0;
    descriptorWrites[0].descriptorType  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    descriptorWrites[0].pBufferInfo     = &bufferInfo;

    descriptorWrites[1].sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrites[1].descriptorCount = static_cast<uint32_t>(MAX_IMAGE_ARRAYS);
    descriptorWrites[1].dstSet          = descriptorSets[i];
    descriptorWrites[1].dstBinding      = 1;
    descriptorWrites[1].dstArrayElement = 0;
    descriptorWrites[1].descriptorType  = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    descriptorWrites[1].pImageInfo      = imageInfos.data();

    vkUpdateDescriptorSets(device, static_cast<uint32_t>(descriptorWrites.size()),
                           descriptorWrites.data(), 0, nullptr);
  }
}

void GraphicCore::copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize bufferSize) {
  VkCommandBuffer commandBuffer = beginSingleTimeCommands();

  VkBufferCopy copyRegion{};
  copyRegion.size = bufferSize;

  vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);
  endSingleTimeCommands(commandBuffer);
}

uint32_t GraphicCore::findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) {
  VkPhysicalDeviceMemoryProperties memProperties;
  vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);

  for (uint32_t i = 0; i != memProperties.memoryTypeCount; ++i) {
    if (typeFilter & (1 << i) &&
        (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
      return i;
    }
  }

  throw std::runtime_error("failed to find suitable memory type!");
}

void GraphicCore::recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex) {
  VkCommandBufferBeginInfo beginInfo{};
  beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

  if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
    throw std::runtime_error("failed to begin recording command buffer!");
  }

  VkRenderPassBeginInfo renderPassInfo{};
  renderPassInfo.sType             = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
  renderPassInfo.renderPass        = renderPass;
  renderPassInfo.framebuffer       = swapChainFramebuffers[imageIndex];
  renderPassInfo.renderArea.offset = {0, 0};
  renderPassInfo.renderArea.extent = swapChainExtent;
  VkClearValue clearColor          = {0.0f, 0.0f, 0.0f, 1.0f};
  renderPassInfo.clearValueCount   = 1;
  renderPassInfo.pClearValues      = &clearColor;

  vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
  vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);

  VkBuffer vertexBuffers[] = {vertexBuffer};
  VkDeviceSize offsets[]   = {0};
  vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);

  vkCmdBindIndexBuffer(commandBuffer, indexBuffer, 0, VK_INDEX_TYPE_UINT16);
  vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1,
                          &descriptorSets[currentFrame], 0, nullptr);

  for (const auto& [_, figure] : figures) {
    PushConstant pc = {.model = figure.model, .textureHandler = figure.textureHandler};
    vkCmdPushConstants(commandBuffer, pipelineLayout,
                       VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(pc),
                       &pc);
    vkCmdDrawIndexed(commandBuffer, figure.indexCount, 1, figure.firstIndex, 0, 0);
  }

  vkCmdEndRenderPass(commandBuffer);

  if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
    throw std::runtime_error("failed to end command buffer recording!");
  }
}

void GraphicCore::createCommandBuffers() {
  commandBuffers.resize(swapChainFramebuffers.size());
  VkCommandBufferAllocateInfo allocInfo{};
  allocInfo.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  allocInfo.commandPool        = commandPool;
  allocInfo.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  allocInfo.commandBufferCount = static_cast<uint32_t>(commandBuffers.size());

  if (vkAllocateCommandBuffers(device, &allocInfo, commandBuffers.data()) != VK_SUCCESS) {
    throw std::runtime_error("failed to create command buffers!");
  }
}

void GraphicCore::createSyncObjects() {
  imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
  renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
  inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);
  imagesInFlight.resize(swapChainImages.size(), VK_NULL_HANDLE);

  VkSemaphoreCreateInfo semaphoreInfo{};
  semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

  VkFenceCreateInfo fenceInfo{};
  fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
  fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

  for (size_t i = 0; i != imageAvailableSemaphores.size(); ++i) {
    if (vkCreateSemaphore(device, &semaphoreInfo, nullptr, &imageAvailableSemaphores[i]) !=
          VK_SUCCESS ||
        vkCreateSemaphore(device, &semaphoreInfo, nullptr, &renderFinishedSemaphores[i]) !=
          VK_SUCCESS ||
        vkCreateFence(device, &fenceInfo, nullptr, &inFlightFences[i]) != VK_SUCCESS) {
      throw std::runtime_error("failed to create semaphores or fences!");
    }
  }
}

VkShaderModule GraphicCore::createShaderModule(const std::vector<char>& code) {
  VkShaderModuleCreateInfo createInfo{};
  createInfo.sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
  createInfo.codeSize = code.size();
  createInfo.pCode    = reinterpret_cast<const uint32_t*>(code.data());
  VkShaderModule shaderModule;
  if (vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
    throw std::runtime_error("failed to create shader module!");
  }
  return shaderModule;
}

bool GraphicCore::checkDeviceExtensionSupport(VkPhysicalDevice device) {
  uint32_t extensionCount;
  vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

  std::vector<VkExtensionProperties> availableExtensions(extensionCount);
  vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount,
                                       availableExtensions.data());

  std::set<std::string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());
  for (const auto& extension : availableExtensions) {
    requiredExtensions.erase(extension.extensionName);
  }
  return requiredExtensions.empty();
}

bool GraphicCore::isDeviceSuitable(VkPhysicalDevice device) {
  VkPhysicalDeviceProperties properties;
  VkPhysicalDeviceFeatures features;
  vkGetPhysicalDeviceProperties(device, &properties);
  vkGetPhysicalDeviceFeatures(device, &features);

  bool extesionsSupported = checkDeviceExtensionSupport(device);

  bool swapChainAdequate = false;
  if (extesionsSupported) {
    SwapChainSupportDetails details = querySwapChainSupport(device);
    swapChainAdequate               = !details.formats.empty() && !details.presentModes.empty();
  }

  QueueFamilyIndicies indicies = findQueueFamilies(device);
  return features.geometryShader && indicies.isComplete() && extesionsSupported &&
         swapChainAdequate && features.samplerAnisotropy;
}

void GraphicCore::pickPhysicalDevice() {
  uint32_t physicalDevicesCount;
  vkEnumeratePhysicalDevices(instance, &physicalDevicesCount, nullptr);

  if (physicalDevicesCount == 0)
    throw std::runtime_error("failed to found GPUs with Vulkan support!");

  std::vector<VkPhysicalDevice> devicesFound(physicalDevicesCount);
  vkEnumeratePhysicalDevices(instance, &physicalDevicesCount, devicesFound.data());

  for (const auto& device : devicesFound) {
    if (isDeviceSuitable(device)) {
      physicalDevice = device;
      break;
    }
  }

  if (physicalDevice == VK_NULL_HANDLE) {
    throw std::runtime_error("failed to find a suitable GPU!");
  }

  VkPhysicalDeviceProperties pDProperties;
  vkGetPhysicalDeviceProperties(physicalDevice, &pDProperties);
#ifndef NDEBUG
  std::cout << "INFO: Current physical device: " << pDProperties.deviceName << "\n";
#endif
}

GraphicCore::QueueFamilyIndicies GraphicCore::findQueueFamilies(VkPhysicalDevice device) {
  QueueFamilyIndicies indicies;

  uint32_t availableQueueFamilyCount = 0;
  vkGetPhysicalDeviceQueueFamilyProperties(device, &availableQueueFamilyCount, nullptr);

  std::vector<VkQueueFamilyProperties> availableQueueFamilies(availableQueueFamilyCount);
  vkGetPhysicalDeviceQueueFamilyProperties(device, &availableQueueFamilyCount,
                                           availableQueueFamilies.data());
  uint32_t i              = 0;
  VkBool32 presentSupport = false;
  for (const auto& family : availableQueueFamilies) {
    if (family.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
      indicies.graphicFamily = i;
    }
    if (!indicies.presentFamily.has_value()) {
      vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);
      if (presentSupport) {
        indicies.presentFamily = i;
      }
    }
    if (indicies.isComplete()) {
      break;
    }
    ++i;
  }

  return indicies;
}

GraphicCore::SwapChainSupportDetails GraphicCore::querySwapChainSupport(VkPhysicalDevice device) {
  SwapChainSupportDetails details;
  vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.capabilities);

  uint32_t formatCount;
  vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);
  if (formatCount != 0) {
    details.formats.resize(formatCount);
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, details.formats.data());
  }

  uint32_t modeCount;
  vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &modeCount, nullptr);
  if (modeCount != 0) {
    details.presentModes.resize(modeCount);
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &modeCount,
                                              details.presentModes.data());
  }

  return details;
}

VkSurfaceFormatKHR GraphicCore::chooseSwapSurfaceFormat(
  const std::vector<VkSurfaceFormatKHR>& availableFormats) {
  for (const auto& availableFormat : availableFormats) {
    if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB &&
        availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
      return availableFormat;
    }
  }

  return availableFormats[0];
}

VkPresentModeKHR GraphicCore::chooseSwapPresentMode(
  const std::vector<VkPresentModeKHR>& availablePresentModes) {
  for (const auto& availablePresentMode : availablePresentModes) {
    if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
      return availablePresentMode;
    }
  }

  return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D GraphicCore::chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities) {
  if (capabilities.currentExtent.width != UINT32_MAX) {
    return capabilities.currentExtent;
  } else {
    int width, height;
    glfwGetFramebufferSize(window, &width, &height);

    VkExtent2D actualExtent = {.width  = static_cast<uint32_t>(width),
                               .height = static_cast<uint32_t>(height)};
    actualExtent.width      = std::max(capabilities.minImageExtent.width,
                                       std::min(capabilities.maxImageExtent.width, actualExtent.width));
    actualExtent.height =
      std::max(capabilities.minImageExtent.height,
               std::min(capabilities.maxImageExtent.height, actualExtent.height));

    return actualExtent;
  }
}

std::vector<const char*> GraphicCore::getRequiredExtensions() {
  uint32_t glfwExtensionCount = 0;
  const char** glfwExtensions;

  glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
  std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

  if (enableValidationLayers) {
    extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
  }

  return extensions;
}

void GraphicCore::createInstance() {
  if (enableValidationLayers && !checkValidationLayerSupport()) {
    throw std::runtime_error("Validation layers enabled, but not supported");
  }

  uint32_t extensionCount = 0;
  vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);

  instanceExtensions.resize(extensionCount);
  vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, instanceExtensions.data());

  VkApplicationInfo appInfo{};
  appInfo.sType              = VK_STRUCTURE_TYPE_APPLICATION_INFO;
  appInfo.pApplicationName   = "Hello Triangle";
  appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
  appInfo.pEngineName        = "No Engine";
  appInfo.engineVersion      = VK_MAKE_VERSION(1, 0, 0);
  appInfo.apiVersion         = VK_API_VERSION_1_0;

  VkInstanceCreateInfo createInfo{};
  createInfo.sType            = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
  createInfo.pApplicationInfo = &appInfo;

  auto extensions = getRequiredExtensions();

  if (!checkExtensionsSupport(extensions.size(), extensions.data())) {
    throw std::runtime_error("Some extensions are requested, but not supported");
  }

  createInfo.enabledExtensionCount   = static_cast<uint32_t>(extensions.size());
  createInfo.ppEnabledExtensionNames = extensions.data();

  VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo;
  if (enableValidationLayers) {
    createInfo.enabledLayerCount   = static_cast<uint32_t>(validationLayers.size());
    createInfo.ppEnabledLayerNames = validationLayers.data();

    populateDebugMessengerCreateInfo(debugCreateInfo);
    createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&debugCreateInfo;
  } else {
    createInfo.enabledLayerCount = 0;

    createInfo.pNext = nullptr;
  }

  if (vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS) {
    throw std::runtime_error("failed to create instance!");
  }
}

bool GraphicCore::checkValidationLayerSupport() {
  uint32_t layerCount;
  vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

  std::vector<VkLayerProperties> availableLayers(layerCount);
  vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());
  for (const char* layerName : validationLayers) {
    bool layerFound = false;

    for (const auto& layerProperties : availableLayers) {
      if (strcmp(layerName, layerProperties.layerName) == 0) {
        layerFound = true;
        break;
      }
    }

    if (!layerFound) {
      return false;
    }
  }

  return true;
}

VKAPI_ATTR VkBool32 VKAPI_CALL GraphicCore::debugCallback(
  VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
  VkDebugUtilsMessageTypeFlagsEXT messageType,
  const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData) {
  const char* severityString;
  if (messageSeverity == VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT) {
    severityString = "ERROR, ";
  } else if (messageSeverity == VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
    severityString = "WARNING, ";
  } else {
    severityString = "VERBOSE, ";
  }
  std::cerr << severityString << "validation layer: " << pCallbackData->pMessage << std::endl;

  return VK_FALSE;
}

void GraphicCore::populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo) {
  createInfo                 = {};
  createInfo.sType           = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
  createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
                               VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                               VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
  createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                           VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                           VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
  createInfo.pfnUserCallback = debugCallback;
  createInfo.pUserData       = nullptr;
}

void GraphicCore::setupDebugMessenger() {
  if (!enableValidationLayers)
    return;
  VkDebugUtilsMessengerCreateInfoEXT createInfo{};
  populateDebugMessengerCreateInfo(createInfo);

  if (CreateDebugUtilsMessengerEXT(instance, &createInfo, nullptr, &debugMessenger) != VK_SUCCESS) {
    throw std::runtime_error("failed to set up debug messenger!");
  }
}