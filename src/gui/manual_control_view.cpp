#include <sstream>

#include "gui/manual_control_view.h"
#include "gui/manual_control_view_controller.h"
#include "schedule/schedule_gpio.h"

manual_control_view::manual_control_view(lv_obj_t *container) : m_container(container) { create_gui(); }

void manual_control_view::create_gui() {
    int screen_width = ::lv_obj_get_width(m_container);
    m_gpio_override_checkbox = lv_cb_create(m_container, nullptr);
    lv_cb_set_text(m_gpio_override_checkbox, "Manual control");
    lv_cb_set_action(m_gpio_override_checkbox, manual_control_view_controller::check_override_schedule);
    lv_obj_set_size(m_gpio_override_checkbox, screen_width - 60, 20);
    lv_obj_align(m_gpio_override_checkbox, m_container, LV_ALIGN_IN_TOP_LEFT, 10, 60);

    m_gpio_control_switch = lv_sw_create(m_container, nullptr);
    lv_sw_set_action(m_gpio_control_switch, manual_control_view_controller::manually_control_gpio);
    lv_obj_set_size(m_gpio_control_switch, (screen_width - lv_obj_get_width(m_gpio_override_checkbox) - 30) / 2,
                    lv_obj_get_height(m_gpio_override_checkbox));
    lv_obj_align(m_gpio_control_switch, m_container, LV_ALIGN_IN_TOP_RIGHT, -10, 60);
    lv_obj_set_hidden(m_gpio_control_switch, true);

    m_gpio_chooser = lv_ddlist_create(m_container, nullptr);
    lv_ddlist_set_hor_fit(m_gpio_chooser, false);
    lv_ddlist_set_anim_time(m_gpio_chooser, LV_DDLIST_ANIM_TIME / 2);
    lv_ddlist_set_action(m_gpio_chooser, manual_control_view_controller::select_gpio_event);
    lv_obj_set_width(m_gpio_chooser, screen_width - 20);
    lv_obj_align(m_gpio_chooser, m_container, LV_ALIGN_IN_TOP_MID, 0, 10);

    lv_ddlist_set_selected(m_gpio_chooser, 0);
}

void manual_control_view::update_contents() {
    std::ostringstream gpio_list_output("");

    auto gpio_id_list = schedule_gpio::get_ids();

    if (gpio_id_list.size() == 0) {
        return;
    }

    for (auto current_gpio_id = gpio_id_list.cbegin(); current_gpio_id != gpio_id_list.cend() - 1; ++current_gpio_id) {
        gpio_list_output << *current_gpio_id << "\n";
    }
    gpio_list_output << gpio_id_list.back();

    size_t len = gpio_list_output.str().size();
    m_gpio_list_len = len + 1;
    m_gpio_list = std::make_unique<char[]>(m_gpio_list_len);
    std::strncpy(m_gpio_list.get(), gpio_list_output.str().c_str(), len + 1);

    lv_ddlist_set_options(m_gpio_chooser, m_gpio_list.get());
    lv_ddlist_set_selected(m_gpio_chooser, 0);
}

lv_obj_t *manual_control_view::container() { return m_container; }

lv_obj_t *manual_control_view::gpio_chooser() { return m_gpio_chooser; }

lv_obj_t *manual_control_view::gpio_control_switch() { return m_gpio_control_switch; }

lv_obj_t *manual_control_view::gpio_override_checkbox() { return m_gpio_override_checkbox; }

const lv_obj_t *manual_control_view::container() const { return m_container; }

const lv_obj_t *manual_control_view::gpio_chooser() const { return m_gpio_chooser; }

const lv_obj_t *manual_control_view::gpio_control_switch() const { return m_gpio_control_switch; }

const lv_obj_t *manual_control_view::gpio_override_checkbox() const { return m_gpio_override_checkbox; }

size_t manual_control_view::gpio_list_string_len() const { return m_gpio_list_len; }
