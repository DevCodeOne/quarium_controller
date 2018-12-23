#include "gui/module_view.h"
#include "gui/module_view_controller.h"
#include "logger.h"
#include "modules/module_collection.h"

module_view::module_view(lv_obj_t *container) : m_container(container) { create_gui(); }

page_event module_view::enter_page() {
    update_contents();
    return page_event::ok;
}

page_event module_view::leave_page() { return page_event::ok; }

page_event module_view::update_contents() { return page_event::ok; }

void module_view::create_gui() {
    auto module_collection_instance = module_collection::instance();

    if (module_collection_instance == nullptr) {
        return;
    }

    std::ostringstream list_of_modules;

    for (auto [id, current_module] : module_collection_instance->get_modules()) {
        list_of_modules << id << '\n';
        switch_to_module(current_module);
    }

    std::string data = list_of_modules.str();
    auto last_pos = data.find_last_of('\n');

    if (last_pos != std::string::npos) {
        data.erase(last_pos);
    }

    // Has to be created afterwards so that the pages don't hide the lowered drop down menu
    m_module_chooser = lv_ddlist_create(m_container, nullptr);
    lv_ddlist_set_hor_fit(m_module_chooser, false);
    lv_obj_set_width(m_module_chooser, lv_obj_get_width(m_container) - 20);
    lv_obj_align(m_module_chooser, m_container, LV_ALIGN_IN_TOP_LEFT, 10, 10);
    lv_ddlist_set_selected(m_module_chooser, module_collection_instance->get_modules().size() - 1);
    lv_ddlist_set_anim_time(m_module_chooser, LV_DDLIST_ANIM_TIME / 2);
    lv_ddlist_set_action(m_module_chooser, module_view_controller::module_selected);

    lv_ddlist_set_options(m_module_chooser, data.c_str());
}

void module_view::switch_to_module(std::shared_ptr<module_interface> module) {
    if (module == nullptr) {
        return;
    }

    if (m_module_views.find(module->id()) == m_module_views.cend()) {
        create_module_page(module);
    }

    for (auto &[id, current_page] : m_module_views) {
        lv_obj_set_hidden(current_page.container(), true);
    }

    if (auto visible_module_element = m_module_views.find(module->id());
        visible_module_element != m_module_views.cend()) {
        lv_obj_set_hidden(visible_module_element->second.container(), false);
    }
}

void module_view::create_module_page(std::shared_ptr<module_interface> module) {
    if (auto result = m_module_views.find(module->id()); result != m_module_views.cend()) {
        return;
    }

    single_module_element module_element(m_container, module);
    lv_obj_set_size(module_element.container(), lv_obj_get_width(m_container) - 20,
                    lv_obj_get_height(m_container) - 70);
    lv_obj_align(module_element.container(), m_container, LV_ALIGN_IN_TOP_LEFT, 10, 60);
    lv_page_set_scrl_fit(module_element.container(), false, true);
    lv_page_set_sb_mode(module_element.container(), LV_SB_MODE_AUTO);

    m_module_views.try_emplace(module->id(), module_element);
}

lv_obj_t *module_view::container() { return m_container; }

const lv_obj_t *module_view::container() const { return m_container; }

single_module_element::single_module_element(lv_obj_t *parent, std::shared_ptr<module_interface> module)
    : m_module(module), m_container(lv_page_create(parent, nullptr)) {
    lv_obj_t *update_continously = lv_cb_create(m_container, nullptr);
    lv_cb_set_text(update_continously, "Instantanious update");
    lv_obj_align(update_continously, m_container, LV_ALIGN_IN_TOP_LEFT, 10, 10);

    for (auto &[key, current_value] : module->description().values()) {
        m_values.emplace_back(m_container, module, current_value);
    }

    if (m_values.size() > 0) {
        lv_obj_align(m_values.front().container(), update_continously, LV_ALIGN_OUT_BOTTOM_LEFT, 10, 10);
        lv_obj_set_size(m_values.front().container(), lv_obj_get_width(m_container) - 20, 40);

        for (auto current_value_element = m_values.begin() + 1; current_value_element != m_values.end();
             ++current_value_element) {
            lv_obj_align(current_value_element->container(), (current_value_element - 1)->container(),
                         LV_ALIGN_OUT_BOTTOM_LEFT, 0, 10);
            lv_obj_set_size(current_value_element->container(), lv_obj_get_width(m_container) - 20, 40);
        }
    }
}

lv_obj_t *single_module_element::container() { return m_container; }

single_module_value_element::single_module_value_element(lv_obj_t *parent, std::shared_ptr<module_interface> module,
                                                         const module_value &value)
    : m_container(lv_cont_create(parent, nullptr)), m_value(value), m_value_storage_id(_value_storage_id++) {
    value_storage<unsigned int, module_value_element_storage>::instance()->change_value(
        m_value_storage_id, module_value_element_storage{module, value.name()});
    m_value_name = lv_label_create(m_container, nullptr);
    lv_label_set_text(m_value_name, m_value.name().c_str());

    switch (value.what_type()) {
        case module_value_type::integer:
        case module_value_type::floating:
            // TODO somehow manage to add floating point values to sliders (maybe with two sliders ?)
            m_value_manipulator = lv_slider_create(m_container, nullptr);
            lv_slider_set_action(m_value_manipulator, single_module_value_element_controller::value_changed);
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
    lv_obj_set_free_num(m_value_manipulator, m_value_storage_id);
    lv_cont_set_fit(m_container, true, true);
}

lv_obj_t *single_module_value_element::container() { return m_container; }
