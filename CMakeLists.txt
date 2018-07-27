project(quarium_controller C CXX)
cmake_minimum_required(VERSION 3.6.2)

add_definitions(-DDEFAULT_CONFIG_PATH="../conf/settings.json")

include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
conan_basic_setup()

add_executable(quarium_controller src/quarium_controller.cpp src/config.cpp src/logger.cpp)
set_property(TARGET quarium_controller PROPERTY CXX_STANDARD 17)
target_compile_options(quarium_controller PUBLIC -Wall -Wextra -Wpedantic)

target_link_libraries(quarium_controller PUBLIC ${CONAN_LIBS})
target_link_libraries(quarium_controller PUBLIC stdc++fs)
target_include_directories(quarium_controller PRIVATE include)
