#pragma once
#include <EventBus.hpp>
#include <entt/entt.hpp>
#include <glm/glm.hpp>
#include <variant>

namespace phys2d {

/* Maths */
constexpr float pi  = 3.14159265;
constexpr float pi2 = pi * 2;
constexpr float inf = 2e5;

/* Precision*/
constexpr float epsilon = 1e-6;

/* RigidBody*/
constexpr float rb_defaultDamping      = 0.01f;
constexpr float rb_defaultRestitution  = 0.5f;
constexpr float rb_defaultFriction     = 0.2f;
constexpr float rb_defaultInvMass      = 1.0f;
constexpr float rb_defaultInvInertia   = 1.0f;
constexpr float rb_defaultGravityScale = 1.0f;

/* Collider */
constexpr glm::vec2 cd_defaultLocalOffset = {0.0f, 0.0f};
constexpr bool cd_defaultTriggerStatus    = false;

enum class BodyType {
  Static,     ///< zero mass, zero velocity, can be manually moved
  Kinematic,  ///< zero mass, velocity is set by user, is moved by System
  Dynamic     ///< non-negative mass, velocity is determined by outer forces, is moved by physics
              ///< system
};

enum class BodyShape {
  Polygon,      ///< convex polygon, has an array of points
  Circle,       ///< perfect geometric circle, has radius
  Capsule,      ///< capsule form, is constructed of two circles with same radius and rectangle
  Segment,      ///< simple segment, has length
  SegmentChain  ///< chain of segments, has array of points to connect sequently
};

struct AABB {
  glm::vec2 lowerBound;  ///< point with the smallest coords
  glm::vec2 upperBound;  ///< point with the biggest coords

  AABB() = default;
  /**
   * @brief constructor for AABB-object
   * @param lb: lower bound, point with the smallest coords
   * @param ub: upper bound, point with the biggest coords
   * @retval None
   */
  AABB(const glm::vec2& lb, const glm::vec2& ub);

  /**
   * @brief  checks the collision between two AABB-objects
   * @param  other: AABB-object to check collision with
   * @retval true if the objects collide and false else
   */
  bool collidesWith(const AABB& other) const;
};

/**
 * @brief  checks the collision between two AABB-objects
 * @param  aabb1: first box
 * @param  aabb2: second box
 * @retval true if the objects collide and false else
 */
bool checkCollision(const AABB& aabb1, const AABB& aabb2);

struct TransformComponent {
  glm::vec2 position{0.0f, 0.0f};  ///< global position of the object on the grid
  float rotation{0.0f};            ///< rotation angle of the object, clockwise
  float scaling{1.0f};             ///< scaling coefficient

  mutable bool dirty{true};             ///< should we update model_matrix or not
  mutable glm::mat3 modelMatrix{1.0f};  ///< model matrix for MVP-rendering

  /**
   * @brief  recalculates the model matrix
   * @param  None
   * @retval None
   */
  void updateMatrix() const;

  /**
   * @brief default concstructor for TransformComponent
   */
  TransformComponent() = default;

  /**
   * @brief constructor for TransformComponent with all fields
   */
  TransformComponent(const glm::vec2& position, float rotation, float scaling);

  /**
   * @brief  gives the forward vector of an object. Can be used for cannons and bullets, for example
   * @param None
   * @retval forward direction
   */
  const glm::vec2 getForward() const;

  /// @name Setters
  /// @{
  /**
   * @brief  sets the new object's position on the grid
   * @param new_position: new position where the object will be located
   * @retval None
   */
  void setPosition(const glm::vec2& new_position);

  /**
   * @brief  sets the new object's rotation in radians, clockwise
   * @param new_angle: new angle of the rotation of the object, in radians
   * @retval None
   */
  void setRotation(const float new_angle);

  /**
   * @brief  sets the new object's scaling coefficients
   * @param new_scale: new scale of the object
   * @retval None
   */
  void setScale(float new_scale);
  /// @}

  /**
   * @brief  changes the object's position: (x, y) -> (x + direction.x, y + direction.y)
   * @note   (0, 0) - center of the screen
   * @note   x-axis is horizontal
   * @note   y-axis is vertical
   * @param  direction: vector of movement
   * @retval None
   */
  void move(const glm::vec2& direction);

  /**
   * @brief  rotates the object
   * @param  angle_delta: value of rotation, given in radians, rotates clockwise
   * @retval None
   */
  void rotate(float angle_delta);

  /**
   * @brief  scales the object
   * @param  scale_coef: scaling coef to multiply
   * @retval None
   */
  void scale(float scale_coef);
};

struct RigidBodyComponent {
  BodyType bodyType = BodyType::Dynamic;  ///< bodyType: Static, Kinematic or Dynamic

  glm::vec2 linearVelocity{0.0f, 0.0f};         ///< velocity along some axis
  glm::vec2 force{0.0f, 0.0f};                  ///< force: two-dimensional vector
  float gravityScale = rb_defaultGravityScale;  ///< gravity scale, 1 by default

  float angularVelocity = 0.0f;  ///< angular velocity, in radians clockwise
  float torque          = 0.0f;  ///< torque, in radians clockwise

  float invMass =
    rb_defaultInvMass;  ///< inverted mass coefficient, used for calculating linear acceleration
  float invInertia = rb_defaultInvInertia;  ///< inverted inertia, used for calculating angular
                                            ///< acceleration, in radians clockwise

  float linearDamping  = rb_defaultDamping;  ///< linear damping of the object, from 0 to 1
  float angularDamping = rb_defaultDamping;  ///< angular damping of the object, from 0 to 1

  float restitution = rb_defaultRestitution;  ///< restitution of the object, 0.5 by default
  float friction    = rb_defaultFriction;     ///< friction of the object, 0.2 by default

  /**
   * @brief default RigidBodyComponent constructor
   */
  RigidBodyComponent() = default;

  /**
   * @brief RigidBodyComponent c-tor that uses only BodyType parameter, other fields are set by
   * default
   * @param bt: BodyType value, Static, Kinematic or Dynamic
   */
  RigidBodyComponent(phys2d::BodyType bt);

  /**
   * @brief RigidBody c-tor, requires all fields to be customly set
   * @param bt: BodyType of the object: Static/Kinematic/Dynamic
   * @param linearVelocity: velocity along some axis
   * @param force: force: two-dimensional vector
   * @param gravityScale: gravity scale, 1 by default
   * @param angularVelocity: angular velocity, in radians clockwise
   * @param torque: torque, in radians, clockwise
   * @param mass: mass of the object
   * @param inertia: inertia of the object, in radians, clockwise
   * @param linearDamping: damping, coefficient from 0 to 1
   * @param angularDamping: angularDamping, coefficient from 0 to 1
   * @param restitiution: physical restitution, 0.5 by default
   * @param friction: physical friction, 0.2 by default
   */
  RigidBodyComponent(BodyType bt, const glm::vec2& linearVelocity, const glm::vec2& force,
                     float gravityScale, float angularVelocity, float torque, float mass,
                     float inertia, float linearDamping, float angularDamping, float restitution,
                     float friction);
};

struct PolygonGeometry {
  std::vector<glm::vec2> vertices;
};

struct CircleGeometry {
  float radius;
};

struct CapsuleGeometry {
  float radius;
  float rectangleLen;
};

struct SegmentGeometry {
  glm::vec2 borders[2];
};

struct SegmentChain {
  std::vector<glm::vec2> points;
};

using ShapeData = std::variant<PolygonGeometry, CircleGeometry>;

namespace ColliderLayers {
constexpr uint32_t None             = 0x0000;
constexpr uint32_t Player           = 0x0001;
constexpr uint32_t PlayerProjectile = 0x0002;
constexpr uint32_t Enemy            = 0x0004;
constexpr uint32_t EnemyProjectile  = 0x0008;
constexpr uint32_t Environment      = 0x0010;
constexpr uint32_t All              = 0xFFFF;
}  // namespace ColliderLayers

struct ColliderComponent {
  BodyShape shapeType;  ///< shape of the object: polygon, circle, segment etc
  ShapeData shapeData;  ///< shape data: vertices, radius, borders etc
  glm::vec2 localOffset = cd_defaultLocalOffset;  ///< local offset of the center of the object
  bool isTrigger = cd_defaultTriggerStatus;  ///< flag that shows whether the object is trigger zone
  uint32_t categoryBits = ColliderLayers::Player;  ///< bitmask that shows the type of the object,
                                                   ///< used for collision filtering
  uint32_t maskBits =
    ColliderLayers::All & ~ColliderLayers::PlayerProjectile;  ///< bitmask, shows the types that the
                                                              ///< object can collide with

  ColliderComponent();
  ColliderComponent(const BodyShape& st);
  template <typename Geometry>
  ColliderComponent(Geometry&& geom, const glm::vec2& offset = cd_defaultLocalOffset,
                    bool trigger          = cd_defaultTriggerStatus,
                    uint32_t categoryBits = ColliderLayers::Player,
                    uint32_t maskBits     = ColliderLayers::All & ~ColliderLayers::PlayerProjectile)
    : localOffset(offset), isTrigger(trigger), categoryBits(categoryBits), maskBits(maskBits) {
    if constexpr (std::is_same_v<std::decay_t<Geometry>, phys2d::PolygonGeometry>) {
      shapeData     = PolygonGeometry();
      shapeType     = phys2d::BodyShape::Polygon;
      *getPolygon() = std::forward<Geometry>(geom);
    } else if constexpr (std::is_same_v<std::decay_t<Geometry>, phys2d::CircleGeometry>) {
      shapeData    = CircleGeometry();
      shapeType    = phys2d::BodyShape::Circle;
      *getCircle() = std::forward<Geometry>(geom);
    }
  }

  /**
   * @brief  getter for accessing the CircleGeometry data
   * @param  None
   * @retval pointer to the geometry type
   */
  CircleGeometry* getCircle();

  /**
   * @brief  getter for accessing the CircleGeometry data
   * @param  None
   * @retval pointer to the geometry type
   */
  const CircleGeometry* getCircle() const;

  /**
   * @brief  getter for accessing the PolygonGeometry data
   * @param  None
   * @retval pointer to the geometry type
   */
  PolygonGeometry* getPolygon();

  /**
   * @brief  getter for accessing the PolygonGeometry data
   * @param  None
   * @retval pointer to the geometry type
   */
  const PolygonGeometry* getPolygon() const;

  /**
   * @brief  getter for accessing the AABB object
   * @param  tc: transform component for getting position, rotation, scaling
   * @retval AABB object
   */
  AABB getAABB(const TransformComponent& tc) const;
};

class PhysicsSystem {
public:
  /**
   * @brief constructor for PhysicsSystem
   * @param registry: reference to the entt registry to manage entities and components
   */
  PhysicsSystem(entt::registry& registry);

  /**
   * @brief  updates the physics simulation for the current frame
   * @param  dt: delta time since the last frame
   * @retval None
   */
  void update(float dt);

  /**
   * @brief  sets the global gravity vector for the physics world
   * @param  g: new two-dimensional gravity vector
   * @retval None
   */
  void setGravity(const glm::vec2& g);

  /**
   * @brief  gets the current global gravity vector
   * @param  None
   * @retval current gravity vector
   */
  glm::vec2 getGravity() const;

  /**
   * @brief  updates the pointer to the entt registry
   * @param  registry: pointer to the new entt registry
   * @retval None
   */
  void setRegistry(entt::registry* registry);

  /**
   * @brief  sets the event bus for dispatching physics-related events (e.g., collisions)
   * @param  eventBus: pointer to the EventBus instance
   * @retval None
   */
  void setEventBus(EventBus* eventBus);

private:
  void integrateForcesAndVelocities(float dt);

  void checkCollisions();

  void processImpulseAndPushback(entt::entity eA, entt::entity eB, const glm::vec2& normal,
                                 float penetration, bool isTriggerA, bool isTriggerB);

  bool collideCircleVsCircle(entt::entity e1, const TransformComponent& tc1,
                             const CircleGeometry& geometry1, entt::entity e2,
                             const TransformComponent& tc2, const CircleGeometry& geometry2);

  bool collideCircleVsPolygon(entt::entity e1, const TransformComponent& tc1,
                              const CircleGeometry& geometry1, entt::entity e2,
                              const TransformComponent& tc2, const PolygonGeometry& geometry2,
                              bool flipNormal);

  bool collidePolygonVsPolygon(entt::entity e1, const TransformComponent& tc1,
                               const PolygonGeometry& geometry1, entt::entity e2,
                               const TransformComponent& tc2, const PolygonGeometry& geometry2);

private:
  glm::vec2 gravity{0.0f, 0.0f};  ///< global gravity vector applied to dynamic bodies
  entt::registry* registry;       ///< pointer to the registry for fetching entities
  EventBus* eventBus;             ///< pointer to the system event bus
};

}  // namespace phys2d