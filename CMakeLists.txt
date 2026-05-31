include(FetchContent)

cmake_minimum_required(VERSION 3.28)
project(PhysicsEngine)

include_directories(inc)

FetchContent_Declare(
  entt
  GIT_REPOSITORY https://github.com/skypjack/entt.git
  GIT_TAG        v3.16.0
)

FetchContent_Declare(
  glm
  GIT_REPOSITORY https://github.com/g-truc/glm.git
)

FetchContent_MakeAvailable(entt)
FetchContent_MakeAvailable(glm)

add_executable(App src/main.cpp src/types.cpp)

target_link_libraries(App PRIVATE EnTT::EnTT glm::glm)
