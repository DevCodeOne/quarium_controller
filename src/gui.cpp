#include <chrono>

#include "gui.h"
#include "logger.h"
#include "lvgl_driver.h"
#include "signal_handler.h"

std::shared_ptr<gui> gui::instance() {
    std::lock_guard<std::recursive_mutex> _instance_guard(_instance_mutex);

    if (_instance) {
        return _instance;
    }

    _instance = std::shared_ptr<gui>(new gui);
    return _instance;
}

void gui::open_gui() {
    std::lock_guard<std::recursive_mutex> _instance_guard(_instance_mutex);

    if (m_is_started) {
        return;
    }

    m_gui_thread = std::thread(gui_loop);
    m_is_started = true;
}

void gui::close_gui() {
    std::lock_guard<std::recursive_mutex> _instance_guard(_instance_mutex);

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

    auto inst = instance();

    if (inst == nullptr) {
        return;
    }

    inst->m_is_initialized = true;

    lv_init();

    lv_disp_drv_init(&inst->m_display_driver);
    lv_indev_drv_init(&inst->m_input_driver);

    inst->m_display_driver.disp_flush = lvgl_driver::flush_buffer;
    inst->m_input_driver.type = LV_INDEV_TYPE_POINTER;
    inst->m_input_driver.read = lvgl_driver::handle_input;

    lv_disp_drv_register(&inst->m_display_driver);
    lv_indev_drv_register(&inst->m_input_driver);

    inst->m_theme = lv_theme_night_init(20, nullptr);
    lv_theme_set_current(inst->m_theme);

    inst->m_screen = lv_obj_create(nullptr, nullptr);
    lv_scr_load(inst->m_screen);

    lv_obj_t *btn = lv_btn_create(inst->m_screen, nullptr);
    lv_btn_set_fit(btn, true, true);
    lv_obj_set_pos(btn, 20, 20);
    lv_obj_t *btn_label = lv_label_create(btn, nullptr);
    lv_label_set_text(btn_label, "Button 1");

    while (!inst->m_should_exit) {
        lv_task_handler();
        lv_tick_inc(10);
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}

gui::~gui() {
    if (!m_is_initialized) {
        return;
    }
}
