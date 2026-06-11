#pragma once

#include <cstdint>

/**
 * @brief Base marker class for all engine events
 */
struct Event {
  /**
   * @brief Virtual destructor to ensure proper polymorphic cleanup
   * @param None
   * @retval None
   */
  virtual ~Event() = default;
};

/**
 * @brief Fired when two solid physical bodies collide
 */
struct CollideEvent : public Event {
  uint32_t entityA;   ///< ID of the entity that initiated the contact
  uint32_t entityB;   ///< ID of the entity that was hit
  float    normal[2]; ///< Collision normal vector (direction of the impact)
  float    force;     ///< Magnitude of the collision impulse

  /**
   * @brief Default constructor for line-by-line initialization
   * @param None
   * @retval None
   */
  CollideEvent() = default;

  /**
   * @brief Initializes a collision event with specific physics data
   * @param a  : ID of the first entity
   * @param b  : ID of the second entity
   * @param nx : X component of the collision normal vector
   * @param ny : Y component of the collision normal vector
   * @param f  : Force of the impact
   * @retval None
   */
  CollideEvent(uint32_t a, uint32_t b, float nx, float ny, float f)
      : entityA(a), entityB(b), force(f) {
    normal[0] = nx;
    normal[1] = ny;
  }
};

/**
 * @brief Represents the state of a trigger zone overlap
 */
enum class TriggerAction {
  ENTER, ///< Entity just entered the trigger zone
  STAY,  ///< Entity is currently inside the trigger zone
  EXIT   ///< Entity just left the trigger zone
};

/**
 * @brief Fired when an entity overlaps with a non-solid trigger zone
 */
struct InterceptionEvent : public Event {
  uint32_t      entityA; ///< ID of the physical entity (e.g., Player)
  uint32_t      entityB; ///< ID of the non-solid trigger zone
  TriggerAction action;  ///< State of the intersection

  /**
   * @brief Default constructor for delayed initialization
   * @param None
   * @retval None
   */
  InterceptionEvent() = default;

  /**
   * @brief Initializes an interception event
   * @param a   : ID of the physical entity
   * @param b   : ID of the trigger zone entity
   * @param act : Current state of the intersection
   * @retval None
   */
  InterceptionEvent(uint32_t a, uint32_t b, TriggerAction act)
      : entityA(a), entityB(b), action(act) {}
};

// Keeping int type for GLFW compatibility
constexpr int INPUT_RELEASE = 0; ///< Key was released
constexpr int INPUT_PRESS   = 1; ///< Key was pressed this frame
constexpr int INPUT_HOLD    = 2; ///< Key is being held down

/**
 * @brief Fired when input from the OS/hardware is received
 */
struct InputEvent : public Event {
  int key;    ///< GLFW key code (e.g., GLFW_KEY_E)
  int action; ///< State of the key (INPUT_PRESS, INPUT_HOLD, or INPUT_RELEASE)

  /**
   * @brief Default constructor
   * @param None
   * @retval None
   */
  InputEvent() = default;

  /**
   * @brief Initializes an input event directly from OS callbacks
   * @param k : GLFW key code
   * @param a : State of the key
   * @retval None
   */
  InputEvent(int k, int a) : key(k), action(a) {}
};

/**
 * @brief Fired when a built-in quest or script is triggered
 */
struct QuestStartEvent : public Event {
  uint32_t questID;  ///< ID of the quest to be executed
  uint32_t entityID; ///< ID of the entity that initiated the quest

  /**
   * @brief Default constructor
   * @param None
   * @retval None
   */
  QuestStartEvent() = default;

  /**
   * @brief Initializes a quest start event
   * @param q : ID of the quest
   * @param e : ID of the initiating entity
   * @retval None
   */
  QuestStartEvent(uint32_t q, uint32_t e) : questID(q), entityID(e) {}
};