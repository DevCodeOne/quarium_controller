project(quarium_controller C CXX)
cmake_minimum_required(VERSION 3.6.2)

add_definitions(-DDEFAULT_CONFIG_PATH="../conf/settings.json")

option(QUARIUM_CONTROLLER_USE_SDL "Use sdl for the gui instead of the framebuffer" OFF)
option(QUARIUM_CONTROLLER_BUILD_TESTS "Build tests for quarium_controller" ON)

include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
conan_basic_setup(NO_OUTPUT_DIRS)

set (SOURCE_FILES
    src/quarium_controller.cpp
    src/config.cpp
    src/logger.cpp
    src/network_interface.cpp
    src/signal_handler.cpp
    src/gpio_handler.cpp
    src/schedule/schedule.cpp
    src/schedule/schedule_action.cpp
    src/schedule/schedule_event.cpp
    src/schedule/schedule_gpio.cpp
    src/schedule/schedule_handler.cpp
    src/gui/view.cpp
    src/gui/view_controller.cpp
    # src/gui/fb_lvgl_driver.cpp
    src/chrono_time.cpp)

IF (NOT QUARIUM_CONTROLLER_USE_SDL)
    set (SOURCE_FILES ${SOURCE_FILES} src/gui/fb_lvgl_driver.cpp)
ELSE()
    find_package(SDL2 REQUIRED)
    set (SOURCE_FILES ${SOURCE_FILES} src/gui/sdl_lvgl_driver.cpp)
    add_definitions(-DUSE_SDL)
ENDIF()

add_executable(quarium_controller
    ${SOURCE_FILES})

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
target_include_directories(quarium_controller PRIVATE include/schedule)

IF (NOT QUARIUM_CONTROLLER_USE_SDL)
    target_link_libraries(quarium_controller PUBLIC ts)
ELSE()
    target_link_libraries(quarium_controller PUBLIC SDL2::SDL2)
ENDIF()

if (QUARIUM_CONTROLLER_BUILD_TESTS)
    enable_testing()
    add_executable(schedule_test tests/schedule_test.cpp
        src/config.cpp
        src/logger.cpp
        src/signal_handler.cpp
        src/gpio_handler.cpp
        src/schedule/schedule.cpp
        src/schedule/schedule_action.cpp
        src/schedule/schedule_event.cpp
        src/schedule/schedule_gpio.cpp
        src/chrono_time.cpp)

    add_executable(chrono_time_test tests/chrono_time_test.cpp
        src/chrono_time.cpp)

    add_executable(ring_buffer_test tests/ring_buffer_test.cpp)


    set_property(TARGET schedule_test PROPERTY CXX_STANDARD 17)
    set_property(TARGET chrono_time_test PROPERTY CXX_STANDARD 17)
    set_property(TARGET ring_buffer_test PROPERTY CXX_STANDARD 17)

    target_link_libraries(schedule_test  PUBLIC ${CONAN_LIBS})
    target_link_libraries(schedule_test PUBLIC stdc++fs)
    target_include_directories(schedule_test PRIVATE include)
    add_test(CatchTests schedule_test)

    target_link_libraries(chrono_time_test  PUBLIC ${CONAN_LIBS})
    target_link_libraries(chrono_time_test PUBLIC stdc++fs)
    target_include_directories(chrono_time_test PRIVATE include)
    add_test(CatchTests chrono_time_test)

    target_include_directories(ring_buffer_test PRIVATE include)
    add_test(CatchTests ring_buffer_test)

ENDIF()
