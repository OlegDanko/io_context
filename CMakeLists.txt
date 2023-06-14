cmake_minimum_required(VERSION 3.5)

project(io LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)
find_package(glfw3 REQUIRED)

set(LIBS
    Threads::Threads
    glfw
    )

file(GLOB SRC
    src/WindowContext/GLFWContext.cpp
)

add_library(io ${SRC})
target_link_libraries(io PUBLIC ${LIBS})
target_include_directories(io PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

find_package(ICU COMPONENTS uc i18n REQUIRED)
add_executable(io_demo src/main.cpp)
target_link_libraries(io_demo io ICU::uc ICU::i18n)


