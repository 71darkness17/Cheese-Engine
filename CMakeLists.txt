cmake_minimum_required(VERSION 3.16)
project(DvizhEngine VERSION 1.0 LANGUAGES CXX)

# Устанавливаем стандарт C++20
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 1. Поиск зависимостей (Vulkan и GLFW)
find_package(Vulkan REQUIRED)
find_package(glfw3 3.3 REQUIRED) 

# 2. Явное перечисление файлов (лучшая практика для CMake)
# Теперь CMake точно увидит изменения и пересоберет проект при создании новых файлов
set(SOURCES
    src/main.cc
    src/graphicCore.cc
    src/renderQueue.cc
)

# 3. Создаем исполняемый файл
add_executable(${PROJECT_NAME} ${SOURCES})

# 4. Указываем путь к заголовочным файлам (папка inc)
target_include_directories(${PROJECT_NAME} PUBLIC inc)

# 5. Линкуем библиотеки
target_link_libraries(${PROJECT_NAME} 
    PRIVATE 
        Vulkan::Vulkan
        glfw
)
