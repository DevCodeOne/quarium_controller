#include <chrono>
#include <cstring>
#include <iomanip>
#include <sstream>

#include "gui/main_view.h"
#include "gui/main_view_controller.h"

#ifdef USE_SDL
#include "gui/sdl_lvgl_driver.h"
#else
#include "gui/fb_lvgl_driver.h"
#endif

#include "ring_buffer.h"
#include "schedule/schedule.h"
#include "signal_handler.h"

std::shared_ptr<main_view> main_view::instance() {
    std::lock_guard<std::recursive_mutex> _instance_guard(_instance_mutex);

    if (_instance) {
        return _instance;
    }

    _instance = std::shared_ptr<main_view>(new main_view);
    return _instance;
}

void main_view::open_view() {
    std::lock_guard<std::recursive_mutex> _instance_guard(_instance_mutex);

    if (m_is_started) {
        return;
    }

    m_view_thread = std::thread(view_loop);
    m_is_started = true;
}

void main_view::close_view() {
    std::lock_guard<std::recursive_mutex> _instance_guard(_instance_mutex);

    if (!m_is_started) {
        return;
    }

    m_should_exit = true;
    if (m_view_thread.joinable()) {
        m_view_thread.join();
    }
    m_is_started = false;
}

void main_view::view_loop() {
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
    lv_obj_align(navigation_buttons, inst->m_content_container, LV_ALIGN_IN_BOTTOM_MID, 0, 0);
    lv_btnm_set_action(navigation_buttons, view_controller::navigation_event);

    inst->m_content_container = lv_cont_create(inst->m_screen, nullptr);
    lv_obj_set_size(inst->m_content_container, screen_width,
                    screen_height - navigation_buttons_height - status_bar_height);
    lv_obj_align(inst->m_content_container, inst->m_screen, LV_ALIGN_IN_TOP_MID, 0, status_bar_height);
    inst->m_status_bar = lv_cont_create(inst->m_screen, nullptr);
    lv_obj_set_size(inst->m_status_bar, screen_width, status_bar_height);
    lv_obj_align(inst->m_status_bar, inst->m_screen, LV_ALIGN_IN_TOP_MID, 0, 0);
    inst->m_clock = lv_label_create(inst->m_status_bar, nullptr);
    lv_obj_set_size(inst->m_clock, 50, status_bar_height - 10);
    lv_obj_align(inst->m_clock, inst->m_status_bar, LV_ALIGN_IN_TOP_MID, -10, 5);
    lv_label_set_align(inst->m_clock, LV_LABEL_ALIGN_CENTER);
    lv_label_set_text(inst->m_clock, "00:00:00");

    inst->create_pages();

    while (!inst->m_should_exit) {
        inst->update_contents(inst->m_current_page);
        lv_task_handler();
        lv_tick_inc(15);
        // TODO add correct way to sleep, so that one iteration takes 15 ms
        std::this_thread::sleep_for(std::chrono::milliseconds(15));
    }

    lv_obj_del(inst->m_screen);
}

void main_view::create_pages() {
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
        int size = (lv_obj_get_width(m_container[(uint8_t)page_index::front]) / 2) - 15;
        lv_obj_set_size(front_buttons[i], size, size);
        lv_obj_t *button_label = lv_label_create(front_buttons[i], nullptr);
        lv_label_set_text(button_label, front_button_titles[i]);
    }

    lv_obj_align(front_buttons[0], m_content_container, LV_ALIGN_IN_TOP_LEFT, 10, 10);
    lv_obj_align(front_buttons[1], front_buttons[0], LV_ALIGN_OUT_RIGHT_MID, 10, 0);
    lv_obj_align(front_buttons[2], front_buttons[0], LV_ALIGN_OUT_BOTTOM_MID, 0, 10);
    lv_obj_align(front_buttons[3], front_buttons[2], LV_ALIGN_OUT_RIGHT_MID, 10, 0);

    // Manual Control
    lv_btn_set_action(front_buttons[(uint8_t)page_index::manual_control], LV_BTN_ACTION_CLICK,
                      view_controller::front_button_event);
    m_manual_view = std::make_shared<manual_control_view>(m_container[(uint8_t)page_index::manual_control]);
}

void main_view::switch_page(const page_index &new_index) {
    lv_obj_set_hidden(m_container[(uint8_t)m_current_page], true);
    m_current_page = new_index;
    update_contents(m_current_page);

    lv_obj_set_hidden(m_container[(uint8_t)m_current_page], false);
    if (auto last_page = m_visited_pages.retrieve_last_element();
        last_page.has_value() && last_page.value() == m_current_page) {
        return;
    }

    m_visited_pages.put(m_current_page);
}

void main_view::update_contents(const page_index &index) {
    std::chrono::hours hours_since_today = duration_since_epoch<std::chrono::hours>() - duration_since_epoch<days>();

    std::chrono::minutes minutes_since_hour =
        duration_since_epoch<std::chrono::minutes>() - duration_since_epoch<std::chrono::hours>();

    std::chrono::seconds seconds_since_minute =
        duration_since_epoch<std::chrono::seconds>() - duration_since_epoch<std::chrono::minutes>();

    std::ostringstream os;
    os << std::setfill('0') << std::setw(2) << hours_since_today.count() << ":" << std::setw(2)
       << minutes_since_hour.count() << ":" << std::setw(2) << seconds_since_minute.count();
    m_time = os.str();
    lv_label_set_text(m_clock, m_time.data());

    if (index == page_index::manual_control && m_manual_view) {
        m_manual_view->update_contents();
    }
}

std::shared_ptr<manual_control_view> main_view::manual_control_view_instance() { return m_manual_view; }

const std::shared_ptr<manual_control_view> main_view::manual_control_view_instance() const { return m_manual_view; }

void main_view::switch_to_last_page() {
    if (m_visited_pages.size() <= 1) {
        return;
    }

    m_visited_pages.remove_last_element();
    if (auto last_page = m_visited_pages.retrieve_last_element(); last_page) {
        switch_page(last_page.value());
    }
}

main_view::~main_view() {
    if (!m_is_initialized) {
        return;
    }
}
