#include "gui/single_output_view_controller.h"
#include "gui/single_output_view.h"
#include "io/output_value.h"
#include "io/outputs.h"
#include "value_storage.h"

#include "logger.h"

lv_res_t single_output_view_controller::override_checkbox_action(lv_obj_t *override_checkbox) {
    auto value_storage_instance = single_output_view::value_storage_type::instance();

    if (value_storage_instance == nullptr) {
        return LV_RES_OK;
    }

    auto value = value_storage_instance->retrieve_value(lv_obj_get_free_num(override_checkbox));
    if (!value.has_value() || value->m_override_checkbox != override_checkbox) {
        logger::instance()->info("Retrieved value is not valid this shouldn't happen");
        return LV_RES_OK;
    }

    if (value->m_override_value == nullptr || value->m_override_checkbox == nullptr) {
        return LV_RES_OK;
    }

    if (!lv_cb_is_checked(override_checkbox)) {
        outputs::restore_control(value->m_output_id);
    } else {
        auto current_state = outputs::current_state(value->m_output_id);

        if (!current_state) {
            return LV_RES_OK;
        }

        set_value_to_current_state(value->m_override_value, current_state.value());
        outputs::override_with(value->m_output_id, current_state.value());
    }

    lv_obj_set_hidden(value->m_override_value, !lv_cb_is_checked(override_checkbox));
    return LV_RES_OK;
}

lv_res_t single_output_view_controller::override_value_action(lv_obj_t *override_value) {
    auto value_storage_instance = single_output_view::value_storage_type::instance();

    if (value_storage_instance == nullptr) {
        return LV_RES_OK;
    }

    auto value = value_storage_instance->retrieve_value(lv_obj_get_free_num(override_value));
    if (!value.has_value() || value->m_override_value != override_value) {
        logger::instance()->info("Not valid");
        return LV_RES_OK;
    }

    if (value->m_override_value == nullptr || value->m_override_value == nullptr) {
        return LV_RES_OK;
    }

    lv_obj_type_t obj_type;
    lv_obj_get_type(value->m_override_value, &obj_type);
    std::string_view obj_type_string(obj_type.type[0]);

    std::optional<output_value> value_to_write;
    if (obj_type_string == "lv_sw") {
        auto current_value = outputs::current_state(value->m_output_id);

        if (!current_value.has_value()) {
            return LV_RES_OK;
        }

        if (current_value->holds_type<switch_output>()) {
            value_to_write = output_value{::lv_sw_get_state(override_value) ? switch_output::on : switch_output::off};
        } else if (current_value->holds_type<tasmota_power_command>()) {
            value_to_write = tasmota_power_command{::lv_sw_get_state(override_value) ? tasmota_power_command::on
                                                                                     : tasmota_power_command::off};
        }
    } else if (obj_type_string == "lv_slider") {
        // TODO improve this interface one shouldn't be able to overwrite min, max values
        int slider_value = lv_slider_get_value(override_value);

        auto current_value = outputs::current_state(value->m_output_id);

        if (!current_value.has_value()) {
            return LV_RES_OK;
        }

        if (!current_value->holds_type<int>()) {
            return LV_RES_OK;
        }

        auto min = current_value->min<int>();
        auto max = current_value->max<int>();

        int min_value = std::numeric_limits<int>::lowest() + 1;
        int max_value = std::numeric_limits<int>::max() - 1;

        if (min.has_value()) {
            min_value = *min;
        }

        if (max.has_value()) {
            max_value = *max;
        }

        float scale = lv_slider_get_value(override_value) / 100.0f;

        int value = (int)((max_value - std::abs(min_value)) * scale + min_value);

        value_to_write = output_value{value, std::optional<int>(min_value), std::optional<int>(max_value)};
    } else {
        return LV_RES_OK;
    }

    if (value_to_write.has_value()) {
        outputs::override_with(value->m_output_id, *value_to_write);
    }

    return LV_RES_OK;
}

bool single_output_view_controller::set_value_to_current_state(lv_obj_t *override_value, const output_value &value) {
    lv_obj_type_t obj_type;
    lv_obj_get_type(override_value, &obj_type);
    std::string_view obj_type_string(obj_type.type[0]);
    if (obj_type_string == "lv_sw") {
        if (value.holds_type<switch_output>()) {
            switch (value.get<switch_output>().value()) {
                case switch_output::on:
                    lv_sw_on(override_value);
                    break;
                default:
                    lv_sw_off(override_value);
                    break;
            }
        } else if (value.holds_type<tasmota_power_command>()) {
            switch (value.get<tasmota_power_command>().value()) {
                case tasmota_power_command::on:
                    lv_sw_on(override_value);
                    break;
                default:
                    lv_sw_off(override_value);
                    break;
            }
        }
        return true;
    } else if (obj_type_string == "lv_slider") {
        // TODO implement
        return true;
    }

    return false;
}
