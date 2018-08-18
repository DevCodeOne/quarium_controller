#include <algorithm>
#include <string_view>

#include "gui/view.h"
#include "gui/view_controller.h"
#include "schedule/schedule_gpio.h"

lv_res_t view_controller::front_button_event(lv_obj_t *obj) {
    auto inst = view::instance();

    if (!inst) {
        return LV_RES_OK;
    }

    lv_obj_t *label = lv_obj_get_child(obj, nullptr);

    if (!label) {
        return LV_RES_OK;
    }

    std::string_view text = lv_label_get_text(label);

    for (uint8_t i = 0; i <= (uint8_t)view::page_index::logs; ++i) {
        if (text == inst->front_button_titles[i]) {
            inst->switch_page(view::page_index(i));
            break;
        }
    }

    return LV_RES_OK;
}

lv_res_t view_controller::navigation_event(lv_obj_t *obj, const char *button_text) {
    auto inst = view::instance();

    if (!inst) {
        return LV_RES_OK;
    }

    std::string_view button_text_string = button_text;

    if (button_text_string == "Back") {
        inst->switch_to_last_page();
    } else if (button_text_string == "Home") {
        inst->switch_page(view::page_index::front);
    } else if (button_text_string == "Config") {
        inst->switch_page(view::page_index::configuration);
    }

    return LV_RES_OK;
}

lv_res_t view_controller::select_gpio_event(lv_obj_t *ddlist) {
    auto inst = view::instance();
    std::unique_ptr<char[]> buffer = std::make_unique<char[]>(inst->m_gpio_list_len);
    lv_ddlist_get_selected_str(ddlist, buffer.get());
    std::string gpio_id(buffer.get());

    if (!schedule_gpio::is_valid_id(gpio_id)) {
        return LV_RES_OK;
    }

	auto overriden = schedule_gpio::is_overriden(gpio_id);
    if (overriden.has_value()) {
        if (overriden.value() == gpio_pin::action::on) {
            lv_sw_on(inst->m_gpio_control_switch);
        } else {
            lv_sw_off(inst->m_gpio_control_switch);
        }

	manually_control_gpio(inst->m_gpio_control_switch);
    }

    lv_cb_set_checked(inst->m_gpio_override_checkbox, overriden.has_value());
    check_override_schedule(inst->m_gpio_override_checkbox);

    return LV_RES_OK;
}

lv_res_t view_controller::manually_control_gpio(lv_obj_t *sw) {
    auto inst = view::instance();
    std::unique_ptr<char[]> buffer = std::make_unique<char[]>(inst->m_gpio_list_len);
    lv_ddlist_get_selected_str(inst->m_gpio_chooser, buffer.get());
    std::string gpio_id(buffer.get());

    if (!schedule_gpio::is_valid_id(gpio_id)) {
        return LV_RES_OK;
    }

    gpio_pin::action override_with;

    if (lv_sw_get_state(sw)) {
        override_with = gpio_pin::action::on;
    } else {
        override_with = gpio_pin::action::off;
    }

    schedule_gpio::override_with(gpio_id, override_with);

    return LV_RES_OK;
}

lv_res_t view_controller::check_override_schedule(lv_obj_t *checkbox) {
    auto inst = view::instance();

    lv_obj_set_hidden(inst->m_gpio_control_switch, !lv_cb_is_checked(checkbox));

    return LV_RES_OK;
}
