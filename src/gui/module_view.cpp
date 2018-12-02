#include "gui/module_view.h"

module_view::module_view(lv_obj_t *container) : m_container(container) { create_gui(); }

void module_view::update_contents() {}

void module_view::create_gui() {
    m_page = lv_page_create(m_container, nullptr);
    lv_obj_set_size(m_page, lv_obj_get_width(m_container), lv_obj_get_height(m_container));
    lv_page_set_scrl_fit(m_page, false, true);
    lv_page_set_sb_mode(m_page, LV_SB_MODE_AUTO);
}

lv_obj_t *module_view::container() { return m_container; }

const lv_obj_t *module_view::container() const { return m_container; }

