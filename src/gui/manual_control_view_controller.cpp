#include <memory>

#include "gui/main_view.h"
#include "gui/manual_control_view_controller.h"
#include "schedule/schedule_gpio.h"

lv_res_t manual_control_view_controller::select_gpio_event(lv_obj_t *ddlist) {
    auto view_instance = main_view::instance();

    if (view_instance == nullptr) {
        return LV_RES_OK;
    }

    auto manual_control_instance = view_instance->manual_control_view_instance();

    if (manual_control_instance == nullptr) {
        return LV_RES_OK;
    }

    std::unique_ptr<char[]> buffer = std::make_unique<char[]>(manual_control_instance->gpio_list_string_len());
    lv_ddlist_get_selected_str(ddlist, buffer.get());
    std::string gpio_id(buffer.get());

    if (!schedule_gpio::is_valid_id(gpio_id)) {
        return LV_RES_OK;
    }

    auto overriden = schedule_gpio::is_overriden(gpio_id);
    if (overriden.has_value()) {
        if (overriden.value() == gpio_pin::action::on) {
            lv_sw_on(manual_control_instance->gpio_control_switch());
        } else {
            lv_sw_off(manual_control_instance->gpio_control_switch());
        }

        manually_control_gpio(manual_control_instance->gpio_control_switch());
    }

    lv_cb_set_checked(manual_control_instance->gpio_override_checkbox(), overriden.has_value());
    check_override_schedule(manual_control_instance->gpio_override_checkbox());

    return LV_RES_OK;
}

lv_res_t manual_control_view_controller::manually_control_gpio(lv_obj_t *sw) {
    auto view_instance = main_view::instance();

    if (view_instance == nullptr) {
        return LV_RES_OK;
    }

    auto manual_control_instance = view_instance->manual_control_view_instance();

    if (manual_control_instance == nullptr) {
        return LV_RES_OK;
    }

    std::unique_ptr<char[]> buffer = std::make_unique<char[]>(manual_control_instance->gpio_list_string_len());
    lv_ddlist_get_selected_str(manual_control_instance->gpio_chooser(), buffer.get());
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

lv_res_t manual_control_view_controller::check_override_schedule(lv_obj_t *checkbox) {
    auto view_instance = main_view::instance();

    if (view_instance == nullptr) {
        return LV_RES_OK;
    }

    auto manual_control_instance = view_instance->manual_control_view_instance();

    if (manual_control_instance == nullptr) {
        return LV_RES_OK;
    }

    bool is_checked = lv_cb_is_checked(checkbox);

    lv_obj_set_hidden(manual_control_instance->gpio_control_switch(), !is_checked);

    std::unique_ptr<char[]> buffer = std::make_unique<char[]>(manual_control_instance->gpio_list_string_len());
    lv_ddlist_get_selected_str(manual_control_instance->gpio_chooser(), buffer.get());
    std::string gpio_id(buffer.get());

    if (!schedule_gpio::is_valid_id(gpio_id)) {
        return LV_RES_OK;
    }

    if (is_checked) {
        schedule_gpio::override_with(gpio_id, lv_sw_get_state(manual_control_instance->gpio_control_switch())
                                                  ? gpio_pin::action::on
                                                  : gpio_pin::action::off);
    } else {
        schedule_gpio::restore_control(gpio_id);
    }

    return LV_RES_OK;
}
