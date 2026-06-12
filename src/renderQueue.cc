#include "renderQueue.hpp"

void RenderQueue::push(RenderCommand command) {
    std::lock_guard<std::mutex> lock(mutex);
    queue.push(std::move(command));
}

bool RenderQueue::pop(RenderCommand& command) {
    std::lock_guard<std::mutex> lock(mutex);
    
    if(queue.empty())
        return false;
    
    command = queue.front();

    queue.pop();

    return true;
}