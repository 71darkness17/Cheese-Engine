#pragma once

#include <algorithm>
#include <cstdint>
#include <functional>
#include <memory>
#include <type_traits>
#include <typeindex>
#include <unordered_map>
#include <vector>

/**
 * @brief A unique identifier for a registered callback
 */
using HandlerId = uint32_t;

/**
 * @brief Abstract event queue interface for type erasure
 */
class IEventQueue {
public:
  virtual ~IEventQueue() = default;

  /**
   * @brief Calls all subscribers and clears the current queue
   * @param None
   * @retval None
   */
  virtual void Dispatch() = 0;

  /**
   * @brief Removes a handler by its unique ID
   * @param id : The unique identifier of the handler to remove
   * @retval None
   */
  virtual void RemoveHandler(HandlerId id) = 0;
};

/**
 * @brief Typed event queue for a specific event type T
 * @tparam T : The type of the event being handled
 */
template <typename T>
class EventQueue : public IEventQueue {
public:
  std::vector<T> queue;  ///< Accumulated events for the current frame
  std::vector<std::pair<HandlerId, std::function<void(const T&)>>>
    handlers;  ///< Registered callbacks paired with their IDs

  /**
   * @brief Moves events to a local buffer, clears the original queue, and notifies subscribers
   * @note Moving the queue prevents iterator invalidation if Publish is called recursively inside a
   * handler
   * @param None
   * @retval None
   */
  void Dispatch() override {
    if (queue.empty())
      return;

    auto currentEvents = std::move(queue);
    queue.clear();

    for (const auto& event : currentEvents) {
      for (const auto& handlerPair : handlers) {
        handlerPair.second(event);
      }
    }
  }

  /**
   * @brief Erases a callback from the handlers list if the ID matches
   * @param id : The unique identifier of the handler to remove
   * @retval None
   */
  void RemoveHandler(HandlerId id) override {
    // std::remove_if moves elements to be deleted to the end, then erase removes them
    auto it = std::remove_if(handlers.begin(), handlers.end(),
                             [id](const auto& pair) { return pair.first == id; });

    if (it != handlers.end()) {
      handlers.erase(it, handlers.end());
    }
  }
};

/**
 * @brief Central event dispatcher connecting isolated engine systems
 */
class EventBus {
private:
  std::unordered_map<std::type_index, std::unique_ptr<IEventQueue>>
    queues;          ///< Map of event types to their queues
  HandlerId nextId;  ///< Counter for generating unique handler IDs

public:
  /**
   * @brief Initializes the EventBus and resets the handler ID counter
   * @param None
   * @retval None
   */
  EventBus() : nextId(0) {
  }

  /**
   * @brief Adds an event to the corresponding type queue for future dispatch
   * @param event : Universal reference to the event being sent
   * @retval None
   */
  template <typename T>
  void Publish(T&& event) {
    using CleanType = std::decay_t<T>;
    auto typeIdx    = std::type_index(typeid(CleanType));

    if (queues.find(typeIdx) == queues.end()) {
      queues[typeIdx] = std::make_unique<EventQueue<CleanType>>();
    }

    auto* q = static_cast<EventQueue<CleanType>*>(queues[typeIdx].get());
    q->queue.push_back(std::forward<T>(event));
  }

  /**
   * @brief Registers a new subscriber callback for a specific event type
   * @param callback : Function handling the constant reference to the event
   * @retval The unique HandlerId required for unsubscription
   */
  template <typename T>
  HandlerId Subscribe(std::function<void(const T&)> callback) {
    using CleanType = std::decay_t<T>;
    auto typeIdx    = std::type_index(typeid(CleanType));

    if (queues.find(typeIdx) == queues.end()) {
      queues[typeIdx] = std::make_unique<EventQueue<CleanType>>();
    }

    auto* q = static_cast<EventQueue<CleanType>*>(queues[typeIdx].get());

    HandlerId id = ++nextId;
    q->handlers.push_back({id, callback});

    return id;
  }

  /**
   * @brief Removes a handler from the event bus across all queues
   * @note Iterates through all abstract queues. Safe and efficient enough since unsubscriptions are
   * rare
   * @param id : The unique identifier of the handler returned by Subscribe
   * @retval None
   */
  void Unsubscribe(HandlerId id) {
    for (auto& [type, q] : queues) {
      q->RemoveHandler(id);
    }
  }

  /**
   * @brief Initiates sequential dispatch of all accumulated events across all queues
   * @param None
   * @retval None
   */
  void DispatchEvents() {
    for (auto& [type, q] : queues) {
      q->Dispatch();
    }
  }
};