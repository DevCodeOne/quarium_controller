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
        logger::instance()->info("Not valid");
        return LV_RES_OK;
    }

    if (value->m_override_value == nullptr || value->m_override_checkbox == nullptr) {
        return LV_RES_OK;
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
        value_to_write = output_value{(::lv_sw_get_state(override_value) ? switch_output::on : switch_output::off)};
    } else if (obj_type_string == "lv_slider") {
        int slider_value = lv_slider_get_value(override_value);  // is from zero to 100 scale to match min max

        value_to_write = output_value{(int)lv_slider_get_value(override_value)};
    } else {
        return LV_RES_OK;
    }

    if (value_to_write.has_value()) {
        outputs::override_with(value->m_output_id, *value_to_write);
    }

    return LV_RES_OK;
}
