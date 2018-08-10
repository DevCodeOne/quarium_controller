#include <chrono>

#include "gui.h"
#include "logger.h"
#include "signal_handler.h"

// TODO use settings to configure those variables and use these values as default
void gui::init_environment_variables() {
    setenv("TSLIB_FBDEVICE", framebuffer, 1);
    setenv("TSLIB_TSDEVICE", tslib_device, 1);
    setenv("TSLIB_CALIBFILE", tslib_calibration_file, 1);
    setenv("TSLIB_CONFFILE", tslib_configuration_file, 1);
}

std::optional<std::shared_ptr<gui>> gui::instance() {
    std::lock_guard<std::recursive_mutex> _instance_guard(_instance_mutex);

    if (_instance) {
        return _instance;
    }

    _instance = std::shared_ptr<gui>(new gui);
    return _instance;
}

void gui::open_gui() {
    if (m_is_started) {
        return;
    }

    m_gui_thread = std::thread(gui_loop);
}

void gui::close_gui() {
    if (!m_is_started) {
        return;
    }

    m_should_exit = true;
    if (m_gui_thread.joinable()) {
        m_gui_thread.join();
    }
    m_is_started = false;
}

void gui::gui_loop() {
    signal_handler::disable_for_current_thread();

    std::lock_guard<std::recursive_mutex> _instance_guard(_instance_mutex);
    auto instance_opt = instance();

    if (!instance_opt) {
        return;
    }

    auto inst = instance_opt.value();

    init_environment_variables();

    inst->m_init = true;

    while (!inst->m_should_exit) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}

gui::~gui() {
    if (!m_init) {
        return;
    }
}
