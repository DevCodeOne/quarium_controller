project(quarium_controller C CXX)
cmake_minimum_required(VERSION 3.6.2)

add_definitions(-DDEFAULT_CONFIG_PATH="../conf/settings.json")

option(USE_SDL "Use sdl for the gui instead of the framebuffer" OFF)
option(GPIOD_STUB "Use stub gpiod wrapper to test the functionality without the need of a real gpio_chip" OFF)
option(BUILD_TESTS "Build tests for quarium_controller" ON)
option(WITH_GUI "Enable the gui" ON)

include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
conan_basic_setup()

set (SOURCE_FILES
    src/quarium_controller.cpp
    src/config.cpp
    src/logger.cpp
    src/signal_handler.cpp
    src/run_configuration.cpp
    src/network/network_interface.cpp
    src/schedule/schedule.cpp
    src/schedule/schedule_action.cpp
    src/schedule/schedule_event.cpp
    src/schedule/schedule_handler.cpp
    src/io/outputs/outputs.cpp
    src/io/outputs/output_interface.cpp
    src/io/outputs/output_value.cpp
    src/io/outputs/output_scheduler.cpp
    src/io/outputs/remote_function/remote_function.cpp
    src/io/outputs/can/can_output.cpp
    src/io/outputs/mqtt/mqtt_output.cpp
    src/io/interfaces/gpio/gpio_chip.cpp
    src/io/interfaces/gpio/gpio_pin.cpp
    src/io/interfaces/can/can.cpp
    src/io/interfaces/mqtt/mqtt.cpp
    src/chrono_time.cpp)

add_definitions(-DMQTT_NS=mqtt_cpp)

IF (GPIOD_STUB)
    add_definitions(-DUSE_GPIOD_STUBS)
ENDIF()

IF (WITH_GUI)
    list(APPEND SOURCE_FILES
        src/gui/main_view.cpp
        src/gui/main_view_controller.cpp
        src/gui/manual_control_view.cpp
        src/gui/single_output_view.cpp
        src/gui/single_output_view_controller.cpp)

    IF (NOT USE_SDL)
        list (APPEND SOURCE_FILES src/gui/fb_lvgl_driver.cpp)
    ELSE()
        find_package(SDL2 REQUIRED)
        list (APPEND SOURCE_FILES src/gui/sdl_lvgl_driver.cpp)
        add_definitions(-DUSE_SDL)
    ENDIF()

ELSE()
    add_definitions(-DNO_GUI)
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

if (BUILD_TESTS)
    enable_testing()
    add_executable(schedule_test tests/schedule_test.cpp
        src/config.cpp
        src/logger.cpp
        src/signal_handler.cpp
        src/schedule/schedule.cpp
        src/schedule/schedule_action.cpp
        src/schedule/schedule_event.cpp
        src/io/outputs/outputs.cpp
        src/io/outputs/output_interface.cpp
        src/io/outputs/output_value.cpp
        src/io/outputs/remote_function/remote_function.cpp
        src/io/interfaces/gpio/gpio_chip.cpp
        src/io/interfaces/gpio/gpio_pin.cpp
        src/io/outputs/output_scheduler.cpp
        src/run_configuration.cpp
        src/chrono_time.cpp)

    add_executable(chrono_time_test tests/chrono_time_test.cpp
        src/chrono_time.cpp)

    add_executable(ring_buffer_test tests/ring_buffer_test.cpp)

    add_executable(value_transitioner_test tests/value_transitioner_test.cpp)

    add_executable(utils_test tests/utils_test.cpp)

    add_executable(output_value_test tests/output_value_test.cpp
        src/run_configuration.cpp
        src/logger.cpp
        src/io/outputs/output_value.cpp)

    set_property(TARGET schedule_test PROPERTY CXX_STANDARD 17)
    set_property(TARGET chrono_time_test PROPERTY CXX_STANDARD 17)
    set_property(TARGET ring_buffer_test PROPERTY CXX_STANDARD 17)
    set_property(TARGET value_transitioner_test PROPERTY CXX_STANDARD 17)
    set_property(TARGET utils_test PROPERTY CXX_STANDARD 17)
    set_property(TARGET output_value_test PROPERTY CXX_STANDARD 17)

    target_link_libraries(schedule_test  PRIVATE ${CONAN_LIBS})
    target_link_libraries(schedule_test PRIVATE stdc++fs)
    target_include_directories(schedule_test PRIVATE include)
    add_test(schedule_t schedule_test)

    target_link_libraries(chrono_time_test  PRIVATE ${CONAN_LIBS})
    target_link_libraries(chrono_time_test PRIVATE stdc++fs)
    target_include_directories(chrono_time_test PRIVATE include)
    add_test(chrono_time_t chrono_time_test)

    target_include_directories(ring_buffer_test PRIVATE include)
    add_test(ring_buffer_t ring_buffer_test)

    target_include_directories(value_transitioner_test PRIVATE include)
    target_link_libraries(value_transitioner_test  PRIVATE ${CONAN_LIBS})
    add_test(value_transitioner_t value_transitioner_test)

    target_include_directories(utils_test PRIVATE include)
    target_link_libraries(utils_test  PRIVATE ${CONAN_LIBS})
    add_test(utils_test_t utils_test)

    target_include_directories(output_value_test PRIVATE include)
    target_link_libraries(output_value_test PRIVATE ${CONAN_LIBS})
    add_test(output_value_test_t output_value_test)

ENDIF()
