#include <types.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <stdexcept>

namespace phys2d {

AABB::AABB(
  const glm::vec2& lb, 
  const glm::vec2& ub): lowerBound(lb), upperBound(ub)
{
  // check that we have correct lb and ub
  if (lb.x > ub.x || lb.y > ub.y) {
    throw std::logic_error("Incorrect AABB borders");
  }
}

bool AABB::collidesWith(const AABB& other) const
{
  return lowerBound.x <= other.upperBound.x &&
         upperBound.x >= other.lowerBound.x &&
         lowerBound.y <= other.upperBound.y &&
         upperBound.y >= other.lowerBound.y;
}

bool checkCollision(const AABB& aabb1, const AABB& aabb2)
{
  return aabb1.collidesWith(aabb2);
}

TransformComponent::TransformComponent(
  const glm::vec2& position, 
  float rotation, 
  float scaling): position(position), rotation(rotation), scaling(scaling), dirty(true)
{

}

void TransformComponent::updateMatrix() const
{
  if (!dirty) return;
  float cosinus = glm::cos(rotation);
  float sinus = glm::sin(rotation);

  modelMatrix = glm::mat3(
        glm::vec3(cosinus * scaling,  -sinus * scaling, 0),
        glm::vec3(sinus * scaling,   cosinus * scaling, 0),
        glm::vec3(position.x,      position.y,    1.0f)
    );
  dirty = false;
}

const glm::vec2 TransformComponent::getForward() const
{
  return glm::vec2(glm::cos(rotation), -glm::sin(rotation));
}

void TransformComponent::setPosition(const glm::vec2& new_position)
{
  position = new_position;
  dirty = true;
}

void TransformComponent::setRotation(float new_angle)
{
  rotation = new_angle;
  dirty = true;
}

void TransformComponent::setScale(float new_scale)
{
  scaling = new_scale;
  dirty = true;
}

void TransformComponent::move(const glm::vec2& direction)
{
  position += direction;
  dirty = true;
}

void TransformComponent::rotate(float angle_delta)
{
  rotation += angle_delta;
  if (abs(rotation) >= phys2d::pi2) {
    int circles = rotation / phys2d::pi2;
    rotation -= circles * phys2d::pi2;
  }
  dirty = true;
}

void TransformComponent::scale(float scale_coef)
{
  scaling *= scale_coef;
  dirty = true;
}

RigidBodyComponent::RigidBodyComponent(BodyType bt): bodyType(bt)
{

}

RigidBodyComponent::RigidBodyComponent(BodyType bt, const glm::vec2& linearVelocity, const glm::vec2& force, 
  float gravityScale, float angularVelocity, float torque, float mass, float inertia, 
  float linearDamping, float angularDamping, float restitution, float friction):
  bodyType(bt), linearVelocity(linearVelocity), force(force),
  gravityScale(gravityScale), angularVelocity(angularVelocity), torque(torque), invMass((mass == 0) ? 0 : 1/mass),
  invInertia((inertia == 0) ? 0 : 1/inertia), linearDamping(linearDamping), angularDamping(angularDamping),
  restitution(restitution), friction(friction)
{
  
}

PolygonGeometry::PolygonGeometry(const std::vector<glm::vec2>& vertices): vertices(vertices)
{
  calculateNormals();
}

void PolygonGeometry::calculateNormals()
{
  normals.clear();
  size_t count = vertices.size();
  normals.resize(count);
  
  for (size_t i = 0; i < count; ++i) {
    glm::vec2 edge = vertices[(i + 1) % count] - vertices[i];
    glm::vec2 normal = glm::normalize(glm::vec2(-edge.y, edge.x));
    normals[i] = normal;
  }
}

ColliderComponent::ColliderComponent(): 
  shapeType(BodyShape::Circle)
{
  CircleGeometry cg;
  cg.radius = 1.0f;
  shapeData = ShapeData(cg);
}

ColliderComponent::ColliderComponent(const BodyShape& st):
  shapeType(st)
{
  if (st == BodyShape::Circle) {
    shapeData = CircleGeometry();
    getCircle()->radius = 1;
    
  } else if (st == BodyShape::Polygon) {
    shapeData = PolygonGeometry();
    getPolygon()->vertices = {{1, 1}, {1, -1}, {-1, -1}, {-1, 1}};
    getPolygon()->calculateNormals();
  } else {
    throw std::logic_error("Incorrect Geometry type!");
  }
}

CircleGeometry* ColliderComponent::getCircle()
{
  return &std::get<CircleGeometry>(shapeData);
}

const CircleGeometry* ColliderComponent::getCircle() const
{
  return &std::get<CircleGeometry>(shapeData);
}

const PolygonGeometry* ColliderComponent::getPolygon() const
{
  return &std::get<PolygonGeometry>(shapeData);
}

PolygonGeometry* ColliderComponent::getPolygon()
{
  return &std::get<PolygonGeometry>(shapeData);
}

AABB ColliderComponent::getAABB(const TransformComponent& tc) const
{
  tc.updateMatrix();
  if (shapeType == BodyShape::Circle) {
    float r = getCircle()->radius;
    return AABB({tc.position.x - r, tc.position.y - r}, {tc.position.x + r, tc.position.y + r});
  } else if (shapeType == BodyShape::Polygon) {
    float x_min = inf, y_min = inf;
    float x_max = -inf, y_max = -inf;
    for (auto vertex : getPolygon()->vertices) {
      glm::vec3 tmp = {vertex.x, vertex.y, 1.0f};
      glm::vec2 modified = glm::vec2(tc.modelMatrix * tmp);
      x_min = glm::min(x_min, modified.x);
      x_max = glm::max(x_max, modified.x);
      y_min = glm::min(y_min, modified.y);
      y_max = glm::max(y_max, modified.y);
    }
    return AABB({x_min, y_min}, {x_max, y_max});
  }
  throw std::logic_error("ColliderComponent has invalid shapeType");
}

PhysicsSystem::PhysicsSystem(entt::registry& registry): registry(&registry)
{

}

void PhysicsSystem::update(float dt)
{
  contacts.clear();
  if (registry == nullptr) {
    return;
  }
  integrateForcesAndVelocities(dt);
  checkCollisions();
  //this->resolveCollisions(dt);
}

void PhysicsSystem::setGravity(const glm::vec2& new_gravity)
{
  gravity = new_gravity;
}

glm::vec2 PhysicsSystem::getGravity() const
{
  return gravity;
}

void PhysicsSystem::integrateForcesAndVelocities(float dt)
{
  auto view = registry->view<RigidBodyComponent, TransformComponent>();
  for (entt::entity entity: view) {
    auto& rb = view.get<RigidBodyComponent>(entity);
    auto& tc = view.get<TransformComponent>(entity);

    if (rb.bodyType == BodyType::Static) continue;

    glm::vec2 acceleration = rb.force * rb.invMass;
    if (rb.bodyType == BodyType::Dynamic) {
        acceleration += gravity * rb.gravityScale;
    }

    rb.linearVelocity += acceleration * dt;

    rb.linearVelocity *= glm::clamp(1.0f - rb.linearDamping * dt, 0.0f, 1.0f);

    tc.move(rb.linearVelocity * dt);

    float angularAcceleration = rb.torque * rb.invInertia;
    rb.angularVelocity += angularAcceleration * dt;
    rb.angularVelocity *= glm::clamp(1.0f - rb.angularDamping * dt, 0.0f, 1.0f);

    tc.rotate(rb.angularVelocity * dt);

    rb.force = glm::vec2(0.0f);
    rb.torque = 0.0f;
  }
}

void PhysicsSystem::checkCollisions()
{
  auto view = registry->view<ColliderComponent, TransformComponent>();
  for (auto it1 = view.begin(); it1 != view.end(); ++it1) {
    for (auto it2 = std::next(it1); it2 != view.end(); ++it2) {
      entt::entity e1 = *it1;
      entt::entity e2 = *it2;

      const ColliderComponent& cc1 = view.get<ColliderComponent>(e1);
      const ColliderComponent& cc2 = view.get<ColliderComponent>(e2);

      // Check whether the entities can collide
      if ((cc1.categoryBits & cc2.maskBits) == 0 || 
          (cc2.categoryBits & cc1.maskBits) == 0) continue;

      const RigidBodyComponent* rbc1 = registry->try_get<RigidBodyComponent>(e1);
      const RigidBodyComponent* rbc2 = registry->try_get<RigidBodyComponent>(e2);
      if (rbc1 != nullptr &&
          rbc2 != nullptr &&
          rbc1->bodyType == BodyType::Static && 
          rbc2->bodyType == BodyType::Static) continue;
      
      const TransformComponent& tc1 = view.get<TransformComponent>(e1);
      const TransformComponent& tc2 = view.get<TransformComponent>(e2);
      
      // Broadphase
      // checking AABB
      if (!cc1.getAABB(tc1).collidesWith(cc2.getAABB(tc2))) continue;

      // Narrowphase
      // check collision by shapes
      if (cc1.shapeType == BodyShape::Polygon && cc2.shapeType == BodyShape::Polygon) {
        //collidePolygonVsPolygon(e1, tc1, *cc1.getPolygon(), e2, tc2, *cc2.getPolygon());
      } else if (cc1.shapeType == BodyShape::Circle && cc2.shapeType == BodyShape::Circle) {
        collideCircleVsCircle(e1, tc1, *cc1.getCircle(), e2, tc2, *cc2.getCircle());
      } else if (cc1.shapeType == BodyShape::Circle && cc2.shapeType == BodyShape::Polygon) {
        //collideCircleVsPolygon(e1, tc1, *cc1.getCircle(), e2, tc2, *cc2.getPolygon(), false);
      } else if (cc1.shapeType == BodyShape::Polygon && cc2.shapeType == BodyShape::Circle) {
        //collideCircleVsPolygon(e2, tc2, *cc2.getCircle(), e1, tc1, *cc1.getPolygon(), true);
      }
    }
  }
}

bool PhysicsSystem::collideCircleVsCircle(
  entt::entity              e1, 
  const TransformComponent& tc1, 
  const CircleGeometry&     geometry1,
  entt::entity              e2, 
  const TransformComponent& tc2, 
  const CircleGeometry&     geometry2)
{
  glm::vec2 from_1_to_2 = tc2.position - tc1.position;
  float distance = glm::sqrt(glm::dot(from_1_to_2, from_1_to_2));
  float radius_sum = geometry1.radius * tc1.scaling + geometry2.radius * tc2.scaling;
  if (distance >= radius_sum) {
    return false;
  }

  CollisionManifold manifold;
  manifold.entityA = e1;
  manifold.entityB = e2;
  manifold.penetration = radius_sum - distance;

  if (distance < phys2d::epsilon) {
    // circles have centers in the same point
    // make syntetic normal
    manifold.normal = glm::vec2(1.0f, 0.0f);
  } else {
    manifold.normal = from_1_to_2 / distance;
  }
  contacts.push_back(manifold);
  return true;
}

} /* phys2d */