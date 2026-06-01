#include <types.hpp>
#include <glm/glm.hpp>

phys2d::AABB::AABB(
  const glm::vec2& lb, 
  const glm::vec2& ub): lowerBound(lb), upperBound(ub)
{

}

const glm::vec2 phys2d::AABB::getLowerBound() const 
{
  return lowerBound;
}

const glm::vec2 phys2d::AABB::getUpperBound() const 
{
  return upperBound;
}

bool phys2d::AABB::collidesWith(const AABB& other)
{
  glm::vec2 left_upper = other.getUpperBound();
  glm::vec2 right_lower = other.getLowerBound();
  glm::vec2 left_lower(left_upper.x, right_lower.y);
  glm::vec2 right_upper(right_lower.x, left_upper.y);

  return getLowerBound().x <= other.getUpperBound().x &&
         getUpperBound().x >= other.getLowerBound().x &&
         getLowerBound().y <= other.getUpperBound().y &&
         getUpperBound().y >= other.getLowerBound().y;
}