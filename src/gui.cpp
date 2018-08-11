#include <array>
#include <chrono>
#include <string>

#include "gui.h"
#include "logger.h"
#include "lvgl_driver.h"
#include "signal_handler.h"

std::shared_ptr<gui> gui::instance() {
    std::lock_guard<std::recursive_mutex> _instance_guard(_instance_mutex);

    if (_instance) {
        return _instance;
    }

    _instance = std::shared_ptr<gui>(new gui());
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

    lv_obj_t *navigation_buttons = lv_btnm_create(inst->m_screen, nullptr);

    const char *buttons[] = {"Back", "Home", "Config", ""};
    lv_btnm_set_map(navigation_buttons, buttons);
    lv_obj_set_size(navigation_buttons, 320, 40);
    lv_obj_align(navigation_buttons, inst->m_screen, LV_ALIGN_IN_BOTTOM_MID, 0, 0);

    std::array<lv_obj_t *, 4> front_buttons{nullptr, nullptr, nullptr, nullptr};
    std::array<std::string, 4> front_buttons_titles{"Schedules", "Manual Control", "Stats", "Logs"};

    for (int i = 0; i < front_buttons.size(); ++i) {
	    front_buttons[i] = lv_btn_create(inst->m_screen, nullptr);
	    int size = (320 / 2) - 15;
	    lv_obj_set_size(front_buttons[i], size, size);
	    lv_obj_t *button_label = lv_label_create(front_buttons[i], nullptr);
	    lv_label_set_text(button_label, front_buttons_titles[i].c_str());
    }

    lv_obj_align(front_buttons[0], inst->m_screen, LV_ALIGN_IN_TOP_LEFT, 10, 10);
    lv_obj_align(front_buttons[1], front_buttons[0], LV_ALIGN_OUT_RIGHT_MID, 10, 0);
    lv_obj_align(front_buttons[2], front_buttons[0], LV_ALIGN_OUT_BOTTOM_MID, 0, 10);
    lv_obj_align(front_buttons[3], front_buttons[2], LV_ALIGN_OUT_RIGHT_MID, 10, 0);

    while (!inst->m_should_exit) {
        lv_task_handler();
        lv_tick_inc(10);
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    lv_obj_del(inst->m_screen);
}

gui::~gui() {
    if (!m_is_initialized) {
        return;
    }
}
