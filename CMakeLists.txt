project(quarium_controller C CXX)
cmake_minimum_required(VERSION 3.6.2)

add_definitions(-DDEFAULT_CONFIG_PATH="../conf/settings.json")

option(QUARIUM_CONTROLLER_BUILD_TESTS "Build tests for quarium_controller" ON)

include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
conan_basic_setup(NO_OUTPUT_DIRS)

add_executable(quarium_controller
    src/quarium_controller.cpp
    src/config.cpp
    src/logger.cpp
    src/network_interface.cpp
    src/signal_handler.cpp
    src/gpio_handler.cpp
    src/schedule.cpp
    src/gui.cpp)

set_property(TARGET quarium_controller PROPERTY CXX_STANDARD 17)
IF (CMAKE_BUILD_TYPE EQUAL "DEBUG")
target_compile_options(quarium_controller PUBLIC -Wall -Wextra -Wpedantic -fsanitize=address)
ENDIF()

IF (CMAKE_BUILD_TYPE EQUAL "DEBUG")
target_link_libraries(quarium_controller PUBLIC asan)
ENDIF()

target_link_libraries(quarium_controller PUBLIC ${CONAN_LIBS})
target_link_libraries(quarium_controller PUBLIC stdc++fs)
target_include_directories(quarium_controller PRIVATE include)

if (QUARIUM_CONTROLLER_BUILD_TESTS)
    enable_testing()
    add_executable(schedule_test tests/schedule_test.cpp
        src/config.cpp
        src/logger.cpp
        src/network_interface.cpp
        src/signal_handler.cpp
        src/gpio_handler.cpp
        src/schedule.cpp
        src/lvgl_driver.cpp)

    set_property(TARGET schedule_test PROPERTY CXX_STANDARD 17)

    target_link_libraries(schedule_test  PUBLIC ${CONAN_LIBS})
    target_link_libraries(schedule_test PUBLIC stdc++fs)
    target_include_directories(schedule_test PRIVATE include)
    add_test(CatchTests schedule_test)
ENDIF()
