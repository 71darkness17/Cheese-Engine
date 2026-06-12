#pragma once

#define GLM_FORCE_RADIANS

#include <glm/glm.hpp>

#include <vector>
#include <cstdlib>
#include <algorithm>
#include <array>
#include <mutex>
#include <queue>

enum RenderCommandType {
    AddRect,
    AddTriangle,
    RemoveFig,
    SetTransform
};

struct RenderCommand {
    RenderCommandType type;

    union {
        struct {
            glm::vec2 position;
            float width;
            float height;
            glm::vec3 color;
            uint32_t index;
        } addRect;

        struct {
            std::array<glm::vec2, 3> positions;
            glm::vec3 color;
            uint32_t index;
        } addTri;

        struct {
            uint32_t index;
        } remove;

        struct {
            uint32_t index;
            glm::mat4 model;
        } transform;
    };
};

class RenderQueue {
public:
    void push(RenderCommand command);

    bool pop(RenderCommand& command);

private:
    std::mutex mutex;
    std::queue<RenderCommand> queue;
};