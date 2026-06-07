#include <types.hpp>
#include <glm/glm.hpp>
#include <stdexcept>

phys2d::AABB::AABB(
  const glm::vec2& lb, 
  const glm::vec2& ub): lowerBound(lb), upperBound(ub)
{
  // check that we have correct lb and ub
  if (lb.x > ub.x || lb.y > ub.y) {
    throw std::logic_error("Incorrect AABB borders");
  }
}

const glm::vec2 phys2d::AABB::getLowerBound() const 
{
  return lowerBound;
}

const glm::vec2 phys2d::AABB::getUpperBound() const 
{
  return upperBound;
}

bool phys2d::AABB::collidesWith(const AABB& other) const
{
  return getLowerBound().x <= other.getUpperBound().x &&
         getUpperBound().x >= other.getLowerBound().x &&
         getLowerBound().y <= other.getUpperBound().y &&
         getUpperBound().y >= other.getLowerBound().y;
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
  float cosinus = glm::cos(rotation);
  float sinus = glm::sin(rotation);

  model_matrix = glm::mat3(
        glm::vec3(cosinus * scaling.x,  -sinus * scaling.y, position.x),
        glm::vec3(sinus * scaling.x,   cosinus * scaling.y, position.y),
        glm::vec3(0.0f,      0.0f,    1.0f)
    );
  dirty = false;
}

bool phys2d::TransformComponent::isDirty() const {
  return dirty;
}

const glm::vec2 phys2d::TransformComponent::getPosition() const
{
  return position;
}

float phys2d::TransformComponent::getRotation() const
{
  return rotation;
}

const glm::vec2 phys2d::TransformComponent::getScale() const
{
  return scaling;
}

const glm::mat3& phys2d::TransformComponent::getModelMatrix() const
{
  if (dirty) {
    updateMatrix();
    dirty = false;
  }
  return model_matrix;
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
    short circles = rotation / (2 * phys2d::pi);
    rotation -= circles * 2 * phys2d::pi;
  }
  dirty = true;
}

void phys2d::TransformComponent::scale(const glm::vec2& scale_coefs)
{
  scaling *= scale_coefs;
  dirty = true;
}
