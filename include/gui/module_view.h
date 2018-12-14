#pragma once

#include <memory>

#include "lvgl.h"

#include "gui/page_interface.h"

class module_view final : public page_interface {
   public:
    module_view(lv_obj_t *container);

    page_event enter_page() override;
    page_event leave_page() override;
    page_event update_contents() override;

    lv_obj_t *container();

    const lv_obj_t *container() const;

   private:
    void create_gui();

    lv_obj_t *m_container = nullptr;
    lv_obj_t *m_module_chooser = nullptr;
    lv_obj_t *m_page = nullptr;
};
