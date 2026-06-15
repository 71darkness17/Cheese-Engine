#include <Events.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <stdexcept>
#include <types.hpp>
namespace phys2d {

AABB::AABB(const glm::vec2& lb, const glm::vec2& ub) : lowerBound(lb), upperBound(ub) {
  // check that we have correct lb and ub
  if (lb.x > ub.x || lb.y > ub.y) {
    throw std::logic_error("Incorrect AABB borders");
  }
}

bool AABB::collidesWith(const AABB& other) const {
  return lowerBound.x <= other.upperBound.x && upperBound.x >= other.lowerBound.x &&
         lowerBound.y <= other.upperBound.y && upperBound.y >= other.lowerBound.y;
}

bool checkCollision(const AABB& aabb1, const AABB& aabb2) {
  return aabb1.collidesWith(aabb2);
}

TransformComponent::TransformComponent(const glm::vec2& position, float rotation, float scaling)
  : position(position), rotation(rotation), scaling(scaling), dirty(true) {
}

void TransformComponent::updateMatrix() const {
  if (!dirty)
    return;
  float cosinus = glm::cos(rotation);
  float sinus   = glm::sin(rotation);

  modelMatrix = glm::mat3(glm::vec3(cosinus * scaling, -sinus * scaling, 0),
                          glm::vec3(sinus * scaling, cosinus * scaling, 0),
                          glm::vec3(position.x, position.y, 1.0f));
  dirty       = false;
}

const glm::vec2 TransformComponent::getForward() const {
  return glm::vec2(glm::cos(rotation), -glm::sin(rotation));
}

void TransformComponent::setPosition(const glm::vec2& new_position) {
  position = new_position;
  dirty    = true;
}

void TransformComponent::setRotation(float new_angle) {
  rotation = new_angle;
  dirty    = true;
}

void TransformComponent::setScale(float new_scale) {
  scaling = new_scale;
  dirty   = true;
}

void TransformComponent::move(const glm::vec2& direction) {
  position += direction;
  dirty = true;
}

void TransformComponent::rotate(float angle_delta) {
  rotation += angle_delta;
  if (abs(rotation) >= phys2d::pi2) {
    int circles = rotation / phys2d::pi2;
    rotation -= circles * phys2d::pi2;
  }
  dirty = true;
}

void TransformComponent::scale(float scale_coef) {
  scaling *= scale_coef;
  dirty = true;
}

RigidBodyComponent::RigidBodyComponent(BodyType bt) : bodyType(bt) {
}

RigidBodyComponent::RigidBodyComponent(BodyType bt, const glm::vec2& linearVelocity,
                                       const glm::vec2& force, float gravityScale,
                                       float angularVelocity, float torque, float mass,
                                       float inertia, float linearDamping, float angularDamping,
                                       float restitution, float friction)
  : bodyType(bt),
    linearVelocity(linearVelocity),
    force(force),
    gravityScale(gravityScale),
    angularVelocity(angularVelocity),
    torque(torque),
    invMass((mass == 0) ? 0 : 1 / mass),
    invInertia((inertia == 0) ? 0 : 1 / inertia),
    linearDamping(linearDamping),
    angularDamping(angularDamping),
    restitution(restitution),
    friction(friction) {
}

ColliderComponent::ColliderComponent() : shapeType(BodyShape::Circle) {
  CircleGeometry cg;
  cg.radius = 1.0f;
  shapeData = ShapeData(cg);
}

ColliderComponent::ColliderComponent(const BodyShape& st) : shapeType(st) {
  if (st == BodyShape::Circle) {
    shapeData           = CircleGeometry();
    getCircle()->radius = 1;

  } else if (st == BodyShape::Polygon) {
    shapeData              = PolygonGeometry();
    getPolygon()->vertices = {{1, 1}, {1, -1}, {-1, -1}, {-1, 1}};
  } else {
    throw std::logic_error("Incorrect Geometry type!");
  }
}

CircleGeometry* ColliderComponent::getCircle() {
  return &std::get<CircleGeometry>(shapeData);
}

const CircleGeometry* ColliderComponent::getCircle() const {
  return &std::get<CircleGeometry>(shapeData);
}

const PolygonGeometry* ColliderComponent::getPolygon() const {
  return &std::get<PolygonGeometry>(shapeData);
}

PolygonGeometry* ColliderComponent::getPolygon() {
  return &std::get<PolygonGeometry>(shapeData);
}

AABB ColliderComponent::getAABB(const TransformComponent& tc) const {
  tc.updateMatrix();
  if (shapeType == BodyShape::Circle) {
    float r = getCircle()->radius;
    return AABB({tc.position.x - r * tc.scaling, tc.position.y - r * tc.scaling},
                {tc.position.x + r * tc.scaling, tc.position.y + r * tc.scaling});
  } else if (shapeType == BodyShape::Polygon) {
    float x_min = inf, y_min = inf;
    float x_max = -inf, y_max = -inf;
    for (auto vertex : getPolygon()->vertices) {
      glm::vec3 tmp      = {vertex.x, vertex.y, 1.0f};
      glm::vec2 modified = glm::vec2(tc.modelMatrix * tmp);
      x_min              = glm::min(x_min, modified.x);
      x_max              = glm::max(x_max, modified.x);
      y_min              = glm::min(y_min, modified.y);
      y_max              = glm::max(y_max, modified.y);
    }
    return AABB({x_min, y_min}, {x_max, y_max});
  }
  throw std::logic_error("ColliderComponent has invalid shapeType");
}

PhysicsSystem::PhysicsSystem(entt::registry& registry) : registry(&registry) {
}

void PhysicsSystem::update(float dt) {
  if (registry == nullptr) {
    return;
  }
  integrateForcesAndVelocities(dt);
  checkCollisions();
  // this->resolveCollisions(dt);
}

void PhysicsSystem::setGravity(const glm::vec2& new_gravity) {
  gravity = new_gravity;
}

glm::vec2 PhysicsSystem::getGravity() const {
  return gravity;
}

void PhysicsSystem::integrateForcesAndVelocities(float dt) {
  auto view = registry->view<RigidBodyComponent, TransformComponent>();
  for (entt::entity entity : view) {
    auto& rb = view.get<RigidBodyComponent>(entity);
    auto& tc = view.get<TransformComponent>(entity);

    if (rb.bodyType == BodyType::Static)
      continue;

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

    rb.force  = glm::vec2(0.0f);
    rb.torque = 0.0f;
  }
}

void PhysicsSystem::checkCollisions() {
  auto view = registry->view<ColliderComponent, TransformComponent>();
  for (auto it1 = view.begin(); it1 != view.end(); ++it1) {
    for (auto it2 = std::next(it1); it2 != view.end(); ++it2) {
      entt::entity e1 = *it1;
      entt::entity e2 = *it2;

      const ColliderComponent& cc1 = view.get<ColliderComponent>(e1);
      const ColliderComponent& cc2 = view.get<ColliderComponent>(e2);

      // Check whether the entities can collide
      if ((cc1.categoryBits & cc2.maskBits) == 0 || (cc2.categoryBits & cc1.maskBits) == 0)
        continue;

      const RigidBodyComponent* rbc1 = registry->try_get<RigidBodyComponent>(e1);
      const RigidBodyComponent* rbc2 = registry->try_get<RigidBodyComponent>(e2);
      if (rbc1 != nullptr && rbc2 != nullptr && rbc1->bodyType == BodyType::Static &&
          rbc2->bodyType == BodyType::Static)
        continue;

      const TransformComponent& tc1 = view.get<TransformComponent>(e1);
      const TransformComponent& tc2 = view.get<TransformComponent>(e2);

      // Broadphase
      // checking AABB
      if (!cc1.getAABB(tc1).collidesWith(cc2.getAABB(tc2)))
        continue;

      // Narrowphase
      // check collision by shapes
      if (cc1.shapeType == BodyShape::Polygon && cc2.shapeType == BodyShape::Polygon) {
        collidePolygonVsPolygon(e1, tc1, *cc1.getPolygon(), e2, tc2, *cc2.getPolygon());
      } else if (cc1.shapeType == BodyShape::Circle && cc2.shapeType == BodyShape::Circle) {
        collideCircleVsCircle(e1, tc1, *cc1.getCircle(), e2, tc2, *cc2.getCircle());
      } else if (cc1.shapeType == BodyShape::Circle && cc2.shapeType == BodyShape::Polygon) {
        collideCircleVsPolygon(e1, tc1, *cc1.getCircle(), e2, tc2, *cc2.getPolygon(), false);
      } else if (cc1.shapeType == BodyShape::Polygon && cc2.shapeType == BodyShape::Circle) {
        collideCircleVsPolygon(e2, tc2, *cc2.getCircle(), e1, tc1, *cc1.getPolygon(), true);
      }
    }
  }
}

bool PhysicsSystem::collideCircleVsCircle(entt::entity e1, const TransformComponent& tc1,
                                          const CircleGeometry& geometry1, entt::entity e2,
                                          const TransformComponent& tc2,
                                          const CircleGeometry& geometry2) {
  glm::vec2 from_1_to_2 = tc2.position - tc1.position;
  float distance        = glm::sqrt(glm::dot(from_1_to_2, from_1_to_2));
  float radius_sum      = geometry1.radius * tc1.scaling + geometry2.radius * tc2.scaling;
  if (distance >= radius_sum) {
    return false;
  }
  bool isTrigger1 = registry->get<ColliderComponent>(e1).isTrigger;
  bool isTrigger2 = registry->get<ColliderComponent>(e2).isTrigger;

  float penetration = radius_sum - distance;
  glm::vec2 collision_normal =
    (distance < phys2d::epsilon) ? glm::vec2(1.0f, 0.0f) : from_1_to_2 / distance;

  processImpulseAndPushback(e1, e2, collision_normal, penetration, isTrigger1, isTrigger2);
  return true;
}

bool PhysicsSystem::collideCircleVsPolygon(entt::entity e1, const TransformComponent& tc1,
                                           const CircleGeometry& geometry1, entt::entity e2,
                                           const TransformComponent& tc2,
                                           const PolygonGeometry& geometry2, bool flipNormal) {
  tc1.updateMatrix();
  tc2.updateMatrix();

  glm::vec2 circle_center = tc1.position;
  float circle_radius     = geometry1.radius * tc1.scaling;

  size_t vertex_count = geometry2.vertices.size();
  std::vector<glm::vec2> world_vertices(vertex_count);
  for (size_t i = 0; i < vertex_count; ++i) {
    glm::vec3 extended = {geometry2.vertices[i].x, geometry2.vertices[i].y, 1.0f};
    world_vertices[i]  = glm::vec2(tc2.modelMatrix * extended);
  }

  float min_overlap          = phys2d::inf;
  glm::vec2 collision_normal = glm::vec2(0.0f);

  // 1. check collision(classic SAT)
  for (size_t i = 0; i < vertex_count; ++i) {
    glm::vec2 edge = world_vertices[(i + 1) % vertex_count] - world_vertices[i];
    glm::vec2 axis = glm::normalize(glm::vec2(-edge.y, edge.x));

    float min_poly_coord = phys2d::inf;
    float max_poly_coord = -phys2d::inf;

    for (const auto& vertex : world_vertices) {
      float projection = glm::dot(vertex, axis);
      min_poly_coord   = glm::min(min_poly_coord, projection);
      max_poly_coord   = glm::max(max_poly_coord, projection);
    }

    float circle_projection = glm::dot(circle_center, axis);
    float min_circle_coord  = circle_projection - circle_radius;
    float max_circle_coord  = circle_projection + circle_radius;

    if (min_poly_coord >= max_circle_coord || min_circle_coord >= max_poly_coord) {
      return false;
    }

    float overlap =
      glm::min(max_poly_coord, max_circle_coord) - glm::max(min_poly_coord, min_circle_coord);
    if (overlap < min_overlap) {
      min_overlap      = overlap;
      collision_normal = axis;
    }
  }

  // 2. look the case where the only vertice is on the edge of the circle
  size_t closest_vertex = 0;
  float min_distance    = phys2d::inf;

  for (size_t i = 0; i < vertex_count; ++i) {
    float distance = glm::distance(circle_center, world_vertices[i]);
    if (distance < min_distance) {
      min_distance   = distance;
      closest_vertex = i;
    }
  }

  glm::vec2 axis    = circle_center - world_vertices[closest_vertex];
  float axis_length = glm::length(axis);

  if (axis_length > phys2d::epsilon) {
    axis = glm::normalize(axis);

    float min_poly_coord = phys2d::inf;
    float max_poly_coord = -phys2d::inf;

    for (const auto& vertex : world_vertices) {
      float projection = glm::dot(vertex, axis);
      min_poly_coord   = glm::min(min_poly_coord, projection);
      max_poly_coord   = glm::max(max_poly_coord, projection);
    }

    float circle_progjection = glm::dot(circle_center, axis);
    float min_circle_coord   = circle_progjection - circle_radius;
    float max_circle_coord   = circle_progjection + circle_radius;

    if (min_poly_coord >= max_circle_coord || min_circle_coord >= max_poly_coord) {
      return false;
    }
    float overlap =
      glm::min(max_circle_coord, max_poly_coord) - glm::max(min_circle_coord, min_poly_coord);
    if (overlap < min_overlap) {
      min_overlap      = overlap;
      collision_normal = axis;
    }
  }

  float penetration = min_overlap;

  glm::vec2 poly_to_circle = circle_center - tc2.position;
  if (flipNormal) {
    if (glm::dot(collision_normal, poly_to_circle) < 0.0f) {
      collision_normal = -collision_normal;
    }
  } else {
    if (glm::dot(collision_normal, poly_to_circle) > 0.0f) {
      collision_normal = -collision_normal;
    }
  }
  bool isTrigger1 = registry->get<ColliderComponent>(e1).isTrigger;
  bool isTrigger2 = registry->get<ColliderComponent>(e2).isTrigger;
  processImpulseAndPushback(e1, e2, collision_normal, penetration, isTrigger1, isTrigger2);
  return true;
}

bool PhysicsSystem::collidePolygonVsPolygon(entt::entity e1, const TransformComponent& tc1,
                                            const PolygonGeometry& geometry1, entt::entity e2,
                                            const TransformComponent& tc2,
                                            const PolygonGeometry& geometry2) {
  tc1.updateMatrix();
  tc2.updateMatrix();

  size_t vertex_count1 = geometry1.vertices.size();
  size_t vertex_count2 = geometry2.vertices.size();

  std::vector<glm::vec2> world_vertices1(vertex_count1);
  std::vector<glm::vec2> world_vertices2(vertex_count2);

  for (size_t i = 0; i < vertex_count1; ++i) {
    glm::vec3 extended = glm::vec3(geometry1.vertices[i].x, geometry1.vertices[i].y, 1.0f);
    world_vertices1[i] = glm::vec2(tc1.modelMatrix * extended);
  }

  for (size_t i = 0; i < vertex_count2; ++i) {
    glm::vec3 extended = glm::vec3(geometry2.vertices[i].x, geometry2.vertices[i].y, 1.0f);
    world_vertices2[i] = glm::vec2(tc2.modelMatrix * extended);
  }

  float min_overlap          = phys2d::inf;
  glm::vec2 collision_normal = glm::vec2(0.0f);

  // helper for checking overlap
  auto checkPolygonOverlap = [&](const std::vector<glm::vec2>& vertices1,
                                 const std::vector<glm::vec2> vertices2) -> bool {
    size_t vertex_count = vertices1.size();
    for (size_t i = 0; i < vertex_count; ++i) {
      glm::vec2 edge = vertices1[(i + 1) % vertex_count] - vertices1[i];
      glm::vec2 axis = glm::normalize(glm::vec2(-edge.y, edge.x));

      float min_poly1_coord = phys2d::inf;
      float max_poly1_coord = -phys2d::inf;
      for (const auto& vertex : vertices1) {
        float projection = glm::dot(vertex, axis);
        min_poly1_coord  = glm::min(min_poly1_coord, projection);
        max_poly1_coord  = glm::max(max_poly1_coord, projection);
      }

      float min_poly2_coord = phys2d::inf;
      float max_poly2_coord = -phys2d::inf;
      for (const auto vertex : vertices2) {
        float projection = glm::dot(vertex, axis);
        min_poly2_coord  = glm::min(min_poly2_coord, projection);
        max_poly2_coord  = glm::max(max_poly2_coord, projection);
      }

      if (min_poly1_coord >= max_poly2_coord || min_poly2_coord >= max_poly1_coord) {
        return false;
      }

      float overlap =
        glm::min(max_poly1_coord, max_poly2_coord) - glm::max(min_poly1_coord, min_poly2_coord);
      if (overlap < min_overlap) {
        min_overlap      = overlap;
        collision_normal = axis;
      }
    }
    return true;
  };

  // check whether polygon1 overlaps polygon2
  if (!checkPolygonOverlap(world_vertices1, world_vertices2))
    return false;

  // check whether polygon2 overlaps polygon1
  if (!checkPolygonOverlap(world_vertices2, world_vertices1))
    return false;

  float penetration = min_overlap;

  glm::vec2 from_1_to_2 = tc2.position - tc1.position;
  if (glm::dot(collision_normal, from_1_to_2) < 0.0f) {
    collision_normal = -collision_normal;
  }

  bool isTrigger1 = registry->get<ColliderComponent>(e1).isTrigger;
  bool isTrigger2 = registry->get<ColliderComponent>(e2).isTrigger;

  processImpulseAndPushback(e1, e2, collision_normal, penetration, isTrigger1, isTrigger2);
  return true;
}

void PhysicsSystem::setRegistry(entt::registry* registry) {
  this->registry = registry;
}

void PhysicsSystem::setEventBus(EventBus* eventBus) {
  this->eventBus = eventBus;
}

void PhysicsSystem::processImpulseAndPushback(entt::entity e1, entt::entity e2,
                                              const glm::vec2& normal, float penetration,
                                              bool isTrigger1, bool isTrigger2) {
  if (isTrigger1 || isTrigger2) {
    if (eventBus) {
      eventBus->Publish(InterceptionEvent(e1, e2, TriggerAction::ENTER));
    }
    return;
  }

  float impulse_magnitude = 0.0f;

  RigidBodyComponent* rb1 = registry->try_get<RigidBodyComponent>(e1);
  RigidBodyComponent* rb2 = registry->try_get<RigidBodyComponent>(e2);

  if (rb1 || rb2) {
    glm::vec2 v1                = rb1 ? rb1->linearVelocity : glm::vec2(0.0f);
    glm::vec2 v2                = rb2 ? rb2->linearVelocity : glm::vec2(0.0f);
    glm::vec2 relative_velocity = v2 - v1;

    float velocity_along_normal = glm::dot(relative_velocity, normal);

    if (velocity_along_normal < 0.0f) {
      float e            = glm::min(rb1 ? rb1->restitution : 1.0f, rb2 ? rb2->restitution : 1.0f);
      float inv_mass_sum = (rb1 ? rb1->invMass : 0.0f) + (rb2 ? rb2->invMass : 0.0f);

      if (inv_mass_sum > phys2d::epsilon) {
        float j = -(1.0f + e) * velocity_along_normal;
        j /= inv_mass_sum;
        impulse_magnitude = j;

        if (rb1 && rb1->bodyType == BodyType::Dynamic)
          rb1->linearVelocity -= normal * (j * rb1->invMass);
        if (rb2 && rb2->bodyType == BodyType::Dynamic)
          rb2->linearVelocity += normal * (j * rb2->invMass);
      }
    }
  }

  float total_inv_mass = (rb1 ? rb1->invMass : 0.0f) + (rb2 ? rb2->invMass : 0.0f);
  if (total_inv_mass > phys2d::epsilon) {
    constexpr float slop    = 0.01f;
    constexpr float percent = 0.4f;
    glm::vec2 correction = normal * (glm::max(penetration - slop, 0.0f) / total_inv_mass * percent);

    if (rb1 && rb1->bodyType == BodyType::Dynamic)
      registry->get<TransformComponent>(e1).move(-correction * rb1->invMass);
    if (rb2 && rb2->bodyType == BodyType::Dynamic)
      registry->get<TransformComponent>(e2).move(correction * rb2->invMass);
  }

  if (eventBus) {
    eventBus->Publish(CollideEvent(e1, e2, normal, impulse_magnitude));
  }
}

}  // namespace phys2d