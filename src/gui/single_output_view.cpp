#include "gui/single_output_view.h"
#include "gui/single_output_view_controller.h"
#include "io/outputs.h"
#include "logger.h"

single_output_view::single_output_view(lv_obj_t *parent, const std::string &output_id)
    : m_container(lv_cont_create(parent, nullptr)),
      m_override_checkbox(lv_cb_create(m_container, nullptr)),
      m_output_id(output_id),
      m_value_storage_id(single_output_view::id++) {
    create_gui(parent);

    auto storage_instance = value_storage_type::instance();
    storage_instance->create_value(
        m_value_storage_id,
        single_output_element_storage{m_output_id, m_container, m_override_checkbox, m_override_value});
}

void single_output_view::create_gui(lv_obj_t *parent) {
    auto stored_value = outputs::current_state(m_output_id);
    if (!stored_value.has_value()) {
        return;
    }

    if (stored_value->holds_type<switch_output>() || stored_value->holds_type<tasmota_power_command>()) {
        m_override_value = lv_sw_create(m_container, nullptr);
        lv_sw_set_action(m_override_value, single_output_view_controller::override_value_action);
        lv_cont_set_layout(m_container, LV_LAYOUT_ROW_T);
    } else if (stored_value->holds_type<int>() || stored_value->holds_type<unsigned int>()) {
        // TODO set max and min values
        m_override_value = lv_slider_create(m_container, nullptr);
        lv_slider_set_action(m_override_value, single_output_view_controller::override_value_action);
        lv_cont_set_layout(m_container, LV_LAYOUT_COL_L);
    } else {
        logger::instance()->warn("Type is not supported by the gui");
    }

    m_container_style = std::make_unique<lv_style_t>();
    lv_style_copy(m_container_style.get(), lv_obj_get_style(m_container));
    m_container_style->body.padding.hor = 0;
    lv_obj_set_width(m_container, lv_obj_get_width(parent) - 30);
    lv_cont_set_style(m_container, m_container_style.get());
    lv_cont_set_fit(m_container, false, true);

    std::string cb_text = std::string(single_output_view::override_text) + m_output_id;
    lv_cb_set_text(m_override_checkbox, cb_text.c_str());
    lv_cb_set_action(m_override_checkbox, single_output_view_controller::override_checkbox_action);
    lv_obj_set_free_num(m_override_checkbox, m_value_storage_id);
    if (m_override_value) {
        lv_obj_set_free_num(m_override_value, m_value_storage_id);
        lv_obj_set_hidden(m_override_value, true);
    }
}

void single_output_view::update_gui() {}

lv_obj_t *single_output_view::container() { return m_container; }
