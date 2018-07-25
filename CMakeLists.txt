project(quarium_controller C CXX)
cmake_minimum_required(VERSION 3.6.2)
add_compile_options(-Wall -Wextra -Wpedantic)

include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
conan_basic_setup()

add_executable(quarium_controller src/quarium_controller.cpp)

target_link_libraries(quarium_controller PUBLIC ${CONAN_LIBS})
target_include_directories(quarium_controller PRIVATE include)
