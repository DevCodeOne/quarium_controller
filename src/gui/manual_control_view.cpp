#include <cstring>
#include <sstream>

#include "gui/manual_control_view.h"
#include "io/outputs.h"

#include "logger.h"

manual_control_view::manual_control_view(lv_obj_t *container) : m_container(container), m_manual_overrides() {
    create_gui();
}

void manual_control_view::create_gui() {
    m_page = lv_page_create(m_container, nullptr);
    lv_obj_set_size(m_page, lv_obj_get_width(m_container), lv_obj_get_height(m_container));
    lv_page_set_scrl_fit(m_page, false, true);
    lv_page_set_sb_mode(m_page, LV_SB_MODE_AUTO);
    lv_page_set_scrl_layout(m_page, LV_LAYOUT_COL_L);

    for (auto current_id : outputs::get_ids()) {
        m_manual_overrides.emplace_back(m_page, current_id);
        lv_page_glue_obj(m_manual_overrides.back().container(), true);
    }
}

page_event manual_control_view::enter_page() { return page_event::ok; }

page_event manual_control_view::leave_page() { return page_event::ok; }

page_event manual_control_view::update_contents() {
    update_override_elements();
    return page_event::ok;
}

void manual_control_view::update_override_elements() {
    for (auto &current_output_view : m_manual_overrides) {
        current_output_view.update_gui();
    }

    if (m_manual_overrides.size() >= 1) {
        lv_obj_align(m_manual_overrides.begin()->container(), m_page, LV_ALIGN_IN_TOP_LEFT, 10, 10);

        for (auto it = m_manual_overrides.begin() + 1; it != m_manual_overrides.cend(); ++it) {
            lv_obj_align(it->container(), (it - 1)->container(), LV_ALIGN_OUT_LEFT_BOTTOM, 0, 10);
        }
    }
}

lv_obj_t *manual_control_view::container() { return m_container; }
