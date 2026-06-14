#pragma once

#define GLM_FORCE_RADIANS

#include <algorithm>
#include <array>
#include <cstdlib>
#include <glm/glm.hpp>
#include <mutex>
#include <queue>
#include <variant>
#include <vector>

enum RenderCommandType { AddRect, AddTriangle, RemoveFig, SetTransform, SetCamera, SetTexture };

struct TextureDescriptor {
  uint32_t arrayId;
  uint32_t layerId;
};

struct RenderCommand {
  RenderCommandType type;

  struct AddRect {
    glm::vec2 position;
    float width;
    float height;
    glm::vec3 color;
    uint32_t index;
  };

  struct AddTri {
    std::array<glm::vec2, 3> positions;
    glm::vec3 color;
    uint32_t index;
  };

  struct Remove {
    uint32_t index;
  };

  struct Transform {
    uint32_t index;
    glm::mat4 model;
  };

  struct Camera {
    glm::vec2 position;
    float zoom;
  };

  struct SetTexture {
    uint32_t index;
    TextureDescriptor textureDesc;
    std::vector<glm::vec2> texCoords;
  };

  using Data = std::variant<AddRect, AddTri, Remove, Transform, Camera, SetTexture>;

  Data data;
};

class RenderQueue {
public:
  void push(RenderCommand command);

  bool pop(RenderCommand& command);

private:
  std::mutex mutex;
  std::queue<RenderCommand> queue;
};