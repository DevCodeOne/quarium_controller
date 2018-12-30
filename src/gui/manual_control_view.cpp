#include <cstring>
#include <sstream>

#include "gui/manual_control_view.h"
#include "gui/manual_control_view_controller.h"
#include "schedule/schedule_output.h"

manual_control_view::output_override_element::output_override_element(lv_obj_t *parent, const std::string &id,
                                                                      const std::string &override_text)
    : m_id(id),
      m_override_checkbox(lv_cb_create(parent, nullptr)),
      m_override_switch(lv_sw_create(parent, nullptr)),
      m_override_text(std::make_unique<char[]>(override_text.size() + 1)) {
    std::strncpy(m_override_text.get(), override_text.data(), override_text.size());
    lv_cb_set_text(m_override_checkbox, m_override_text.get());
}

lv_obj_t *manual_control_view::output_override_element::override_switch() { return m_override_switch; }

lv_obj_t *manual_control_view::output_override_element::override_checkbox() { return m_override_checkbox; }

const lv_obj_t *manual_control_view::output_override_element::override_switch() const { return m_override_switch; }

const lv_obj_t *manual_control_view::output_override_element::override_checkbox() const { return m_override_checkbox; }

const char *manual_control_view::output_override_element::text() const { return (const char *)m_override_text.get(); }

const std::string &manual_control_view::output_override_element::id() const { return m_id; }

manual_control_view::manual_control_view(lv_obj_t *container) : m_container(container), m_manual_overrides() {
    create_gui();
}

void manual_control_view::create_gui() {
    m_page = lv_page_create(m_container, nullptr);
    lv_obj_set_size(m_page, lv_obj_get_width(m_container), lv_obj_get_height(m_container));
    lv_page_set_scrl_fit(m_page, false, true);
    lv_page_set_sb_mode(m_page, LV_SB_MODE_AUTO);

    update_override_elements();
}

page_event manual_control_view::enter_page() { return page_event::ok; }

page_event manual_control_view::leave_page() { return page_event::ok; }

page_event manual_control_view::update_contents() {
    update_override_elements();
    return page_event::ok;
}

void manual_control_view::update_override_elements() {
    recreate_override_elements();

    for (auto &current_override_element : m_manual_overrides) {
        auto is_overriden = schedule_output::is_overriden(current_override_element.id());

        lv_obj_set_hidden(current_override_element.override_switch(), !is_overriden);

        if (!is_overriden.has_value() || !is_overriden->get<switch_output>().has_value()) {
            continue;
        }

        auto action_value = is_overriden->get<switch_output>();

        if (!action_value.has_value()) {
            continue;
        }

        switch (*action_value) {
            case switch_output::off:
                lv_sw_off(current_override_element.override_switch());
                break;
            case switch_output::on:
                lv_sw_on(current_override_element.override_switch());
                break;
            default:
                lv_sw_off(current_override_element.override_switch());
        }
    }
}
void manual_control_view::recreate_override_elements() {
    auto gpio_id_list = schedule_output::get_ids();

    if (gpio_id_list.size() == 0) {
        return;
    }

    bool missing_id = std::any_of(m_manual_overrides.cbegin(), m_manual_overrides.cend(),
                                  [&gpio_id_list](const auto &current_override_element) -> bool {
                                      return std::find(gpio_id_list.cbegin(), gpio_id_list.cend(),
                                                       current_override_element.id()) == gpio_id_list.cend();
                                  });

    // If all gpio_ids of the manual_overrides are found and the number of elements in both lists is the same every
    // entry is contained in the other list and vice versa, so for the override_elements to be outdated one or more
    // entries have to be missing from the gpiod_id_list or the size has to be different
    if (missing_id || m_manual_overrides.size() == gpio_id_list.size()) {
        // No update necessary gui is up to date
        return;
    }

    if (m_manual_overrides.size() > 0) {
        lv_obj_clean(m_page);
        m_manual_overrides.clear();
    }

    std::ostringstream override_text_stream;
    override_text_stream << "Override " << *gpio_id_list.cbegin();

    output_override_element current_override_element(m_page, *gpio_id_list.cbegin(), override_text_stream.str());
    lv_obj_set_height(current_override_element.override_switch(),
                      lv_obj_get_height(current_override_element.override_checkbox()));
    lv_obj_align(current_override_element.override_checkbox(), m_page, LV_ALIGN_IN_TOP_LEFT, 10, 10);
    lv_obj_align(current_override_element.override_switch(), m_page, LV_ALIGN_IN_TOP_RIGHT, -10, 18);
    lv_sw_set_action(current_override_element.override_switch(), manual_control_view_controller::toggle_switch);
    lv_cb_set_action(current_override_element.override_checkbox(),
                     manual_control_view_controller::check_override_schedule);

    m_manual_overrides.emplace_back(std::move(current_override_element));
    // Reuse std::ostringstream the correct way
    override_text_stream.clear();
    override_text_stream.str(std::string());

    for (auto current_gpio_id = gpio_id_list.cbegin() + 1; current_gpio_id != gpio_id_list.cend(); ++current_gpio_id) {
        override_text_stream << "Override " << *current_gpio_id;

        output_override_element current_override_element(m_page, *current_gpio_id, override_text_stream.str());
        lv_obj_set_height(current_override_element.override_switch(),
                          lv_obj_get_height(current_override_element.override_checkbox()));

        lv_obj_align(current_override_element.override_checkbox(), m_manual_overrides.back().override_checkbox(),
                     LV_ALIGN_OUT_BOTTOM_LEFT, 0, 10);
        lv_obj_align(current_override_element.override_switch(), m_manual_overrides.back().override_switch(),
                     LV_ALIGN_OUT_BOTTOM_RIGHT, 0, 10);
        lv_sw_set_action(current_override_element.override_switch(), manual_control_view_controller::toggle_switch);
        lv_cb_set_action(current_override_element.override_checkbox(),
                         manual_control_view_controller::check_override_schedule);

        m_manual_overrides.emplace_back(std::move(current_override_element));
        override_text_stream.clear();
        override_text_stream.str(std::string());
    }
}

lv_obj_t *manual_control_view::container() { return m_container; }
