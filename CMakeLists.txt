cmake_minimum_required(VERSION 3.28)
project(CheeseEngine VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)

FetchContent_Declare(
    entt
    GIT_REPOSITORY https://github.com/skypjack/entt.git
    GIT_TAG        v3.16.0
)

FetchContent_Declare(
    glm
    GIT_REPOSITORY https://github.com/g-truc/glm.git
)

FetchContent_MakeAvailable(entt glm)

find_package(Vulkan REQUIRED)
find_package(glfw3 3.3 REQUIRED) 

set(SOURCES
    src/main.cpp 
    src/graphicCore.cc
    src/renderQueue.cc
    src/types.cpp
)

add_executable(${PROJECT_NAME} ${SOURCES})

target_include_directories(${PROJECT_NAME} PUBLIC inc)

target_link_libraries(${PROJECT_NAME} 
    PRIVATE 
        Vulkan::Vulkan
        glfw
        EnTT::EnTT
        glm::glm
)

# find_package(Doxygen)
# if(DOXYGEN_FOUND)
#     configure_file(${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile.in ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile @ONLY)

#     add_custom_target(docs
#         COMMAND ${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile
#         WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
#         COMMENT "Generating API documentation with Doxygen"
#         VERBATIM
#     )
# else()
#     message(WARNING "Doxygen not found. Documentation will not be built.")
# endif()
