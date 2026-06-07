#include <types.hpp>
#include <glm/glm.hpp>
#include <stdexcept>
#include <cmath>

inline float phys2d::round(float x) {
    return std::round(x * 1'000'000.f) / 1'000'000.f;
}

phys2d::AABB::AABB(
  const glm::vec2& lb, 
  const glm::vec2& ub): lowerBound(lb), upperBound(ub)
{
  // check that we have correct lb and ub
  if (lb.x > ub.x || lb.y > ub.y) {
    throw std::logic_error("Incorrect AABB borders");
  }
}

bool phys2d::AABB::collidesWith(const AABB& other) const
{
  return lowerBound.x <= other.upperBound.x &&
         upperBound.x >= other.lowerBound.x &&
         lowerBound.y <= other.upperBound.y &&
         upperBound.y >= other.lowerBound.y;
}

bool phys2d::checkCollision(const AABB& aabb1, const AABB& aabb2)
{
  return aabb1.collidesWith(aabb2);
}

phys2d::TransformComponent::TransformComponent(
  const glm::vec2& position, 
  float rotation, 
  const glm::vec2& scaling): position(position), rotation(rotation), scaling(scaling), dirty(true)
{

}

void phys2d::TransformComponent::updateMatrix() const
{
  if (!dirty) return;
  float cosinus = glm::cos(rotation);
  float sinus = glm::sin(rotation);

  model_matrix = glm::mat3(
        glm::vec3(cosinus * scaling.x,  -sinus * scaling.y, position.x),
        glm::vec3(sinus * scaling.x,   cosinus * scaling.y, position.y),
        glm::vec3(0.0f,      0.0f,    1.0f)
    );
  dirty = false;
}

const glm::vec2 phys2d::TransformComponent::getForward() const
{
  return glm::vec2(glm::cos(rotation), -glm::sin(rotation));
}

void phys2d::TransformComponent::setPosition(const glm::vec2& new_position)
{
  position = new_position;
  dirty = true;
}

void phys2d::TransformComponent::setRotation(float new_angle)
{
  rotation = new_angle;
  dirty = true;
}

void phys2d::TransformComponent::setScale(const glm::vec2& new_scale)
{
  scaling = new_scale;
  dirty = true;
}

void phys2d::TransformComponent::move(const glm::vec2& direction)
{
  position += direction;
  dirty = true;
}

void phys2d::TransformComponent::rotate(float angle_delta)
{
  rotation += angle_delta;
  if (rotation >= 2 * phys2d::pi) {
    int circles = rotation / (2 * phys2d::pi);
    rotation -= circles * 2 * phys2d::pi;
  }
  if (rotation <= -2 * phys2d::pi) {
    int circles = abs(rotation / 2 * phys2d::pi);
    rotation += circles * 2 * phys2d::pi;
  }
  dirty = true;
}

void phys2d::TransformComponent::scale(const glm::vec2& scale_coefs)
{
  scaling *= scale_coefs;
  dirty = true;
}