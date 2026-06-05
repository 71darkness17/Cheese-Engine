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

void phys2d::TransformComponent::updateMatrix() {
  
}
