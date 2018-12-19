#include "gui/module_view.h"
#include "modules/module_collection.h"

#include "logger.h"

module_view::module_view(lv_obj_t *container) : m_container(container) { create_gui(); }

page_event module_view::enter_page() { return page_event::ok; }

page_event module_view::leave_page() { return page_event::ok; }

page_event module_view::update_contents() {
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

    return page_event::ok;
}

void module_view::create_gui() {
    m_page = lv_page_create(m_container, nullptr);
    m_module_chooser = lv_ddlist_create(m_container, nullptr);
    lv_ddlist_set_hor_fit(m_module_chooser, false);
    lv_ddlist_set_anim_time(m_module_chooser, 100);
    lv_obj_set_width(m_module_chooser, lv_obj_get_width(m_container) - 20);
    lv_obj_align(m_module_chooser, m_container, LV_ALIGN_IN_TOP_LEFT, 10, 10);

    lv_obj_set_size(m_page, lv_obj_get_width(m_container) - 20,
                    lv_obj_get_height(m_container) - lv_obj_get_height(m_module_chooser) - 30);
    lv_obj_align(m_page, m_module_chooser, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 10);
    lv_page_set_scrl_fit(m_page, false, true);
    lv_page_set_sb_mode(m_page, LV_SB_MODE_AUTO);
}

lv_obj_t *module_view::container() { return m_container; }

const lv_obj_t *module_view::container() const { return m_container; }

