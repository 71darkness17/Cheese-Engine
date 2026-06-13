cmake_minimum_required(VERSION 3.16)
project(DvizhEngine VERSION 1.0 LANGUAGES CXX)


set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


find_package(Vulkan REQUIRED)
find_package(glfw3 3.3 REQUIRED) 


set(SOURCES
    src/main.cc
    src/graphicCore.cc
    src/renderQueue.cc
)


add_executable(${PROJECT_NAME} ${SOURCES})


target_include_directories(${PROJECT_NAME} PUBLIC inc)


target_link_libraries(${PROJECT_NAME} 
    PRIVATE 
        Vulkan::Vulkan
        glfw
)
