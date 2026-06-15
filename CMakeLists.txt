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

FetchContent_Declare(
glfw
GIT_REPOSITORY https://github.com/glfw/glfw.git
GIT_TAG 3.4
)

FetchContent_MakeAvailable(entt glm glfw)

find_package(Vulkan REQUIRED) 

set(SOURCES
    src/graphicCore.cc
    src/renderQueue.cc
    src/types.cpp
)

add_library(${PROJECT_NAME} SHARED
    ${SOURCES}
)

target_compile_definitions(
    CheeseEngine
    PRIVATE
        $<$<CONFIG:Release>:NDEBUG>
)

target_include_directories(${PROJECT_NAME} PUBLIC 
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/inc>
    $<INSTALL_INTERFACE:include>
)

target_link_libraries(${PROJECT_NAME} 
    PRIVATE
        Vulkan::Vulkan
    PUBLIC
        glfw
        EnTT::EnTT
        glm::glm
)

install(
TARGETS CheeseEngine
EXPORT CheeseEngineTargets

RUNTIME DESTINATION bin
LIBRARY DESTINATION lib
ARCHIVE DESTINATION lib

)

install(
DIRECTORY inc/
DESTINATION include
)

install(
EXPORT CheeseEngineTargets
FILE CheeseEngineConfig.cmake
NAMESPACE Cheese::
DESTINATION lib/cmake/CheeseEngine
)

set(CPACK_PACKAGE_NAME CheeseEngine)

set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})

if(WIN32)

set(CPACK_GENERATOR NSIS)

else()

set(CPACK_GENERATOR DEB)

set(
CPACK_DEBIAN_PACKAGE_DEPENDS
"libvulkan1"
)

endif()

include(CPack)



find_package(Doxygen)
if(DOXYGEN_FOUND)
    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile @ONLY)

    add_custom_target(docs
        COMMAND ${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        COMMENT "Generating API documentation with Doxygen"
        VERBATIM
    )
else()
    message(WARNING "Doxygen not found. Documentation will not be built.")
endif()