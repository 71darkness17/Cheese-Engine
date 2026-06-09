#pragma once
#include <glm/glm.hpp>
#include <variant>
namespace phys2d {

/* Trigonometry */
constexpr float pi = 3.14159265;
constexpr float pi2 = pi*2;

/* Precision*/
constexpr float epsilon = 1e-6;

/* RigidBody*/
constexpr float rb_defaultDamping = 0.01f;
constexpr float rb_defaultRestitution = 0.5f;
constexpr float rb_defaultFriction = 0.2f;
constexpr float rb_defaultInvMass = 1.0f;
constexpr float rb_defaultInvInertia = 1.0f;
constexpr float rb_defaultGravityScale = 1.0f;

/* Collider */
constexpr glm::vec2 cd_defaultLocalOffset = {0.0f, 0.0f};
constexpr bool cd_defaultTriggerStatus = false;

enum class BodyType {
  Static,    ///< zero mass, zero velocity, can be manually moved
  Kinematic, ///< zero mass, velocity is set by user, is moved by System
  Dynamic    ///< non-negative mass, velocity is determined by outer forces, is moved by physics system
};

enum class BodyShape {
  Polygon,     ///< convex polygon, has an array of points
  Circle,      ///< perfect geometric circle, has radius
  Capsule,     ///< capsule form, is constructed of two circles with same radius and connecting rectangle
  Segment,     ///< simple segment, has length
  SegmentChain ///< chain of segments, has array of points to connect sequently
};

struct AABB {
  glm::vec2 lowerBound; ///< point with the smallest coords
  glm::vec2 upperBound; ///< point with the biggest coords

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
  glm::vec2 position{0.0f, 0.0f};       ///< global position of the object on the grid
  float rotation{0.0f};                 ///< rotation angle of the object, clockwise
  glm::vec2 scaling{1.0f};              ///< scaling coefficients for x-axis and y-axis respectively
  
  mutable bool dirty{true};             ///< should we update model_matrix or not
  mutable glm::mat3 modelMatrix{1.0f}; ///< model matrix for MVP-rendering

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
  TransformComponent(const glm::vec2& position, float rotation, const glm::vec2& scaling);

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
   * @param new_scale: new scale of the object along x-axis and y-axis respectively
   * @retval None
   */
  void setScale(const glm::vec2& new_scale);
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
   * @param  scale_coefs: scaling coefs to multiply
   * @retval None
   */
  void scale(const glm::vec2& scale_coefs);
};

struct RigidBodyComponent {
  BodyType bodyType = BodyType::Dynamic;       ///< bodyType: Static, Kinematic or Dynamic

  glm::vec2 linearVelocity{0.0f, 0.0f};        ///< velocity along some axis
  glm::vec2 force{0.0f, 0.0f};                 ///< force: two-dimensional vector
  float gravityScale = rb_defaultGravityScale; ///< gravity scale, 1 by default

  float angularVelocity = 0.0f;                ///< angular velocity, in radians clockwise
  float torque = 0.0f;                         ///< torque, in radians clockwise

  float invMass = rb_defaultInvMass;           ///< inverted mass coefficient, used for calculating linear acceleration
  float invInertia = rb_defaultInvInertia;     ///< inverted inertia, used for calculating angular acceleration, in radians clockwise

  float linearDamping = rb_defaultDamping;     ///< linear damping of the object, from 0 to 1
  float angularDamping = rb_defaultDamping;    ///< angular damping of the object, from 0 to 1

  float restitution = rb_defaultRestitution;   ///< restitution of the object, 0.5 by default
  float friction = rb_defaultFriction;         ///< friction of the object, 0.2 by default

  /**
   * @brief default RigidBodyComponent constructor
   */
  RigidBodyComponent() = default;

  /**
   * @brief RigidBodyComponent c-tor that uses only BodyType parameter, other fields are set by default
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
    float gravityScale, float angularVelocity, float torque, float mass, float inertia, 
    float linearDamping, float angularDamping, float restitution, float friction);
};

struct PolygonGeometry {
  std::vector<glm::vec2> vertices;
  std::vector<glm::vec2> normals;
  void calculateNormals();
};

struct CircleGeometry {
  float radius;
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
}

struct ColliderComponent {
  BodyShape shapeType; ///< shape of the object: polygon, circle, segment etc
  ShapeData shapeData; ///< shape data: vertices, radius, borders etc
  glm::vec2 localOffset = cd_defaultLocalOffset;
  bool isTrigger = cd_defaultTriggerStatus;
  uint32_t categoryBits = ColliderLayers::Player;
  uint32_t maskBits = ColliderLayers::All;

  ColliderComponent();
  ColliderComponent(const BodyShape& st);
  template <typename Geometry>
  ColliderComponent(Geometry&& geom, const glm::vec2& offset = cd_defaultLocalOffset, bool trigger = cd_defaultTriggerStatus);

  AABB getAABB(const TransformComponent& tc) const;
  const CircleGeometry* getCircle() const;
  const PolygonGeometry* getPolygon() const;
};

} /* PhysicsEngine */