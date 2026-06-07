#pragma once
#include <glm/glm.hpp>

namespace phys2d {

inline constexpr double pi = 3.141592653589793238462;
inline constexpr double epsilon = 1e-6;
float round(float n);

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
  mutable glm::mat3 model_matrix{1.0f}; ///< model matrix for MVP-rendering

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

class RigidBodyComponent {
private:
  BodyType bodyType;            ///< type of the object, used for movement
  glm::vec2 linearVelocity;     ///< direction of movement 
  float angularVelocity;        ///< direction of angular movement
  glm::vec2 linearAcceleration; ///< acceleration
  float angularAcceleration;    ///< WOWOWOWOW angular acceleration
  float mass;                   ///< mass of the object(needed if the real physical laws are enabled)

public:

};

class ColliderComponent {
private:
  
  BodyShape shapeType; ///< shape of the object: polygon, circle, segment etc

  union ShapeData { ///< Data
    struct { std::vector<glm::vec2> vertices;   } polygon;
    struct { float radius;                      } circle;
    struct { float rectangle_len; float radius; } capsule;
    struct { glm::vec2 borders;                 } segment;
    struct { std::vector<glm::vec2> chain;      } segmentChain;
  } shapeData;

public:
  AABB getAABB();

  /// @name Gettters
  /// @{
  const std::vector<glm::vec2> getPolygonVertices() const;
  const float getCircleRadius() const;
  const float getCapsuleRadius() const;
  const float getCapsuleLength() const;
  const glm::vec2 getSegmentBorders() const;
  const std::vector<glm::vec2> getSegmentChainBorders() const;
  /// @}

  /// @name Setters
  /// @{
  void  getPolygonVertices(const std::vector<glm::vec2>& vertices);
  void  getCircleRadius(const float radius);
  void  getCapsuleRadius(const float radius);
  void  getCapsuleLength(const float length);
  void  getSegmentBorders(const glm::vec2& borders);
  void  getSegmentChainBorders(const std::vector<glm::vec2>& borders);
  /// @}
};

} /* PhysicsEngine */