#include <memory>

#include "gui/main_view.h"
#include "gui/manual_control_view_controller.h"
#include "schedule/schedule_output.h"

lv_res_t manual_control_view_controller::toggle_switch(lv_obj_t *sw) {
    auto view_instance = main_view::instance();

    if (view_instance == nullptr) {
        return LV_RES_OK;
    }

    auto manual_control_instance = view_instance->manual_control_view_instance();

    if (manual_control_instance == nullptr) {
        return LV_RES_OK;
    }

    auto override_element = std::find_if(
        manual_control_instance->m_manual_overrides.cbegin(), manual_control_instance->m_manual_overrides.cend(),
        [sw](const auto &current_override_element) { return current_override_element.override_switch() == sw; });

    if (override_element == manual_control_instance->m_manual_overrides.cend()) {
        return LV_RES_OK;
    }

    auto gpio_id = override_element->id();

    if (!schedule_output::is_valid_id(gpio_id)) {
        return LV_RES_OK;
    }

    schedule_output::override_with(gpio_id, lv_sw_get_state(sw) ? gpio_pin::action::on : gpio_pin::action::off);

    return LV_RES_OK;
}

lv_res_t manual_control_view_controller::check_override_schedule(lv_obj_t *checkbox) {
    auto view_instance = main_view::instance();

    if (view_instance == nullptr) {
        return LV_RES_OK;
    }

    auto manual_control_instance = view_instance->manual_control_view_instance();

    if (manual_control_instance == nullptr) {
        return LV_RES_OK;
    }

    auto override_element = std::find_if(manual_control_instance->m_manual_overrides.begin(),
                                         manual_control_instance->m_manual_overrides.end(),
                                         [checkbox](const auto &current_override_element) {
                                             return current_override_element.override_checkbox() == checkbox;
                                         });

    if (override_element == manual_control_instance->m_manual_overrides.cend()) {
        return LV_RES_OK;
    }

    auto gpio_id = override_element->id();

    if (!schedule_output::is_valid_id(gpio_id)) {
        return LV_RES_OK;
    }

    bool is_checked = lv_cb_is_checked(checkbox);
    lv_obj_set_hidden(override_element->override_switch(), !is_checked);

    if (is_checked) {

        // TODO somehow remove this element, because its id is not valid
        auto current_state = schedule_output::current_state(gpio_id);
        if (!current_state.has_value()) {
            return LV_RES_OK;
        }

        switch (current_state.value()) {
            case gpio_pin::action::on:
                lv_sw_on(override_element->override_switch());
                break;
            default:
                lv_sw_off(override_element->override_switch());
                break;
        }

        schedule_output::override_with(gpio_id, current_state.value());
    } else {
        schedule_output::restore_control(gpio_id);
    }

    return LV_RES_OK;
}
