#pragma once

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "lvgl.h"

#include "gui/page_interface.h"
#include "gui/single_output_view.h"

class manual_control_view final : public page_interface {
   public:
    manual_control_view(lv_obj_t *container);

    page_event enter_page() override;
    page_event leave_page() override;
    page_event update_contents() override;

    lv_obj_t *container();

    const lv_obj_t *container() const;

   private:
    void create_gui();
    void update_override_elements();

    lv_obj_t *m_container = nullptr;
    std::vector<single_output_view> m_manual_overrides;
    lv_obj_t *m_page = nullptr;

    friend class manual_control_view_controller;
};
