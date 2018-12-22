#include "gui/module_view.h"
#include "gui/module_view_controller.h"
#include "modules/module_collection.h"

#include "logger.h"

module_view::module_view(lv_obj_t *container) : m_container(container) { create_gui(); }

page_event module_view::enter_page() { return page_event::ok; }

page_event module_view::leave_page() { return page_event::ok; }

page_event module_view::update_contents() { return page_event::ok; }

void module_view::create_gui() {
    m_module_chooser = lv_ddlist_create(m_container, nullptr);
    lv_ddlist_set_hor_fit(m_module_chooser, false);
    lv_ddlist_set_anim_time(m_module_chooser, 100);
    lv_ddlist_set_action(m_module_chooser, module_view_controller::module_selected);
    lv_obj_set_width(m_module_chooser, lv_obj_get_width(m_container) - 20);
    lv_obj_align(m_module_chooser, m_container, LV_ALIGN_IN_TOP_LEFT, 10, 10);
    update_contents();
    lv_ddlist_set_selected(m_module_chooser, 0);

    auto module_collection_instance = module_collection::instance();

    if (module_collection_instance == nullptr) {
        lv_ddlist_set_options(m_module_chooser, "");
    }

    std::ostringstream list_of_modules;

    for (auto current_module : module_collection_instance->get_modules()) {
        list_of_modules << current_module.first << '\n';
    }

    std::string data = list_of_modules.str();
    auto last_pos = data.find_last_of('\n');

    if (last_pos != std::string::npos) {
        data.erase(last_pos);
    }

    lv_ddlist_set_options(m_module_chooser, data.c_str());
}

void module_view::switch_to_module(std::shared_ptr<module_interface> module) {
    if (module == nullptr) {
        return;
    }

    if (m_pages.find(module->id()) == m_pages.cend()) {
        create_module_page(module);
    }

    for (auto &[id, current_page] : m_pages) {
        lv_obj_set_hidden(current_page, true);
    }

    lv_obj_set_hidden(m_pages[module->id()], false);
}

void module_view::create_module_page(std::shared_ptr<module_interface> module) {
    lv_obj_t *page = lv_page_create(m_container, nullptr);

    lv_obj_set_size(page, lv_obj_get_width(m_container) - 20,
                    lv_obj_get_height(m_container) - lv_obj_get_height(m_module_chooser) - 30);
    lv_obj_align(page, m_module_chooser, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 10);
    lv_page_set_scrl_fit(page, false, true);
    lv_page_set_sb_mode(page, LV_SB_MODE_AUTO);

    lv_obj_t *update_continously = lv_cb_create(page, nullptr);
    lv_cb_set_text(update_continously, "Instantanious update");
    lv_obj_align(update_continously, page, LV_ALIGN_IN_TOP_LEFT, 10, 10);

    auto &current_value_container = m_values[module->id()];
    for (auto &[key, current_value] : module->description().values()) {
        current_value_container.emplace_back(module_single_value_element(page, current_value));
    }

    if (current_value_container.size() > 0) {
        lv_obj_align(current_value_container.front().container(), update_continously, LV_ALIGN_OUT_BOTTOM_LEFT, 10, 10);
        lv_obj_set_size(current_value_container.front().container(), lv_obj_get_width(page) - 20, 40);
    }

    m_pages.emplace(module->id(), page);
}

lv_obj_t *module_view::container() { return m_container; }

const lv_obj_t *module_view::container() const { return m_container; }

module_view::module_single_value_element::module_single_value_element(lv_obj_t *parent, const module_value &value)
    : m_container(lv_cont_create(parent, nullptr)), m_value(value) {
    m_value_name = lv_label_create(m_container, nullptr);
    lv_label_set_text(m_value_name, m_value.name().c_str());

    switch (value.what_type()) {
        case module_value_type::integer:
        case module_value_type::floating:
            // TODO somehow manage to add floating point values to sliders (maybe with two sliders ?)
            m_value_manipulator = lv_slider_create(m_container, nullptr);
            break;
        case module_value_type::string:
        default:
            m_value_manipulator = nullptr;
    }

    if (m_value.what_type() == module_value_type::integer) {
        lv_slider_set_range(m_value_manipulator, value.min<int>(), value.max<int>());
    }

    if (m_value.what_type() == module_value_type::floating) {
        lv_slider_set_range(m_value_manipulator, value.min<float>(), value.max<float>());
    }

    lv_obj_align(m_value_name, m_container, LV_ALIGN_IN_TOP_LEFT, 10, 10);
    lv_obj_align(m_value_manipulator, m_value_name, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 10);
    lv_cont_set_fit(m_container, true, true);
}

lv_obj_t *module_view::module_single_value_element::container() { return m_container; }
