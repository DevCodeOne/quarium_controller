#include <chrono>
#include <cstring>
#include <sstream>

#include "gui.h"
#include "logger.h"
#include "lvgl_driver.h"
#include "schedule.h"
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
    const char *buttons[] = {"Back", "Home", "Config", ""};
    lv_obj_t *navigation_buttons = lv_btnm_create(inst->m_screen, nullptr);
    lv_btnm_set_map(navigation_buttons, buttons);
    lv_obj_set_size(navigation_buttons, screen_width, navigation_buttons_height);
    lv_obj_align(navigation_buttons, inst->m_screen, LV_ALIGN_IN_BOTTOM_MID, 0, 0);
    lv_btnm_set_action(navigation_buttons, gui::navigation_event);

    inst->m_content_container = lv_cont_create(inst->m_screen, nullptr);
    lv_obj_set_size(inst->m_content_container, screen_width, screen_height - navigation_buttons_height);
    lv_obj_align(inst->m_content_container, inst->m_screen, LV_ALIGN_IN_TOP_MID, 0, 0);

    inst->create_pages();

    while (!inst->m_should_exit) {
        lv_task_handler();
        lv_tick_inc(10);
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    lv_obj_del(inst->m_screen);
}

lv_res_t gui::front_button_event(lv_obj_t *obj) {
    auto inst = instance();

    if (!inst) {
        return LV_RES_OK;
    }

    lv_obj_t *label = lv_obj_get_child(obj, nullptr);

    if (!label) {
        return LV_RES_OK;
    }

    std::string_view text = lv_label_get_text(label);

    for (uint8_t i = 0; i <= (uint8_t)page_index::logs; ++i) {
        if (text == front_button_titles[i]) {
            inst->switch_page(page_index(i));
            break;
        }
    }

    return LV_RES_OK;
}

lv_res_t gui::navigation_event(lv_obj_t *obj, const char *button_text) {
    auto inst = instance();

    if (!inst) {
        return LV_RES_OK;
    }

    std::string_view button_text_string = button_text;

    if (button_text_string == "Back") {
        inst->switch_to_last_page();
    } else if (button_text_string == "Home") {
        inst->switch_page(page_index::front);
    } else if (button_text_string == "Config") {
        inst->switch_page(page_index::configuration);
    }

    return LV_RES_OK;
}

void gui::create_pages() {
    for (uint8_t i = 0; i <= (uint8_t)page_index::front; ++i) {
        m_container[i] = lv_cont_create(m_content_container, nullptr);
        lv_obj_set_hidden(m_container[i], true);
        lv_obj_set_size(m_container[i], lv_obj_get_width(m_content_container), lv_obj_get_height(m_content_container));
        lv_obj_align(m_container[i], m_content_container, LV_ALIGN_IN_TOP_MID, 0, 0);
        lv_cont_set_layout(m_container[i], LV_LAYOUT_OFF);
        lv_cont_set_fit(m_container[i], false, false);
    }
    switch_page(page_index::front);

    std::array<lv_obj_t *, 4> front_buttons{nullptr, nullptr, nullptr, nullptr};

    for (int i = 0; i < front_buttons.size(); ++i) {
        front_buttons[i] = lv_btn_create(m_container[(uint8_t)page_index::front], nullptr);
        int size = (screen_width / 2) - 15;
        lv_obj_set_size(front_buttons[i], size, size);
        lv_obj_t *button_label = lv_label_create(front_buttons[i], nullptr);
        lv_label_set_text(button_label, front_button_titles[i]);
    }

    lv_obj_align(front_buttons[0], m_screen, LV_ALIGN_IN_TOP_LEFT, 10, 10);
    lv_obj_align(front_buttons[1], front_buttons[0], LV_ALIGN_OUT_RIGHT_MID, 10, 0);
    lv_obj_align(front_buttons[2], front_buttons[0], LV_ALIGN_OUT_BOTTOM_MID, 0, 10);
    lv_obj_align(front_buttons[3], front_buttons[2], LV_ALIGN_OUT_RIGHT_MID, 10, 0);

    m_gpio_chooser = lv_ddlist_create(m_container[(uint8_t)page_index::manual_control], nullptr);
    lv_ddlist_set_hor_fit(m_gpio_chooser, false);
    lv_ddlist_set_anim_time(m_gpio_chooser, LV_DDLIST_ANIM_TIME / 2);
    lv_obj_set_width(m_gpio_chooser, screen_width - 20);
    lv_obj_align(m_gpio_chooser, m_container[(uint8_t)page_index::manual_control], LV_ALIGN_IN_TOP_MID, 0, 10);

    lv_btn_set_action(front_buttons[(uint8_t)page_index::manual_control], LV_BTN_ACTION_CLICK, gui::front_button_event);
}

void gui::switch_page(const page_index &new_index) {
    lv_obj_set_hidden(m_container[(uint8_t)current_page], true);
    current_page = new_index;
    update_contents(current_page);

    lv_obj_set_hidden(m_container[(uint8_t)current_page], false);
    if (m_visited_pages.size() > 0 && current_page == m_visited_pages.back()) {
        return;
    }

    m_visited_pages.emplace_back(current_page);
}

void gui::update_contents(const page_index &index) {
    if (index == page_index::manual_control) {
        std::ostringstream gpio_list_output("");

        auto gpio_id_list = schedule_gpio::get_ids();

        if (gpio_id_list.size() == 0) {
            return;
        }

        for (auto current_gpio_id = gpio_id_list.cbegin(); current_gpio_id != gpio_id_list.cend() - 1;
             ++current_gpio_id) {
            gpio_list_output << *current_gpio_id << "\n";
        }
        gpio_list_output << gpio_id_list.back();

        size_t len = gpio_list_output.str().size();
        m_gpio_list = std::make_unique<char[]>(len + 1);
        std::strncpy(m_gpio_list.get(), gpio_list_output.str().c_str(), len + 1);

        lv_ddlist_set_options(m_gpio_chooser, m_gpio_list.get());
        lv_ddlist_set_selected(m_gpio_chooser, 0);
    }
}

void gui::switch_to_last_page() {
    if (m_visited_pages.size() <= 1) {
        return;
    }

    m_visited_pages.erase(m_visited_pages.cend() - 1);
    switch_page(m_visited_pages.back());
}

gui::~gui() {
    if (!m_is_initialized) {
        return;
    }
}
