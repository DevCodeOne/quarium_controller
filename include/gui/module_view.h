#pragma once

#include <memory>

#include "lvgl.h"

class module_view {
   public:
    module_view(lv_obj_t *container);
    void update_contents();

    lv_obj_t *container();

    const lv_obj_t *container() const;

   private:

    void create_gui();

    lv_obj_t *m_container = nullptr;
    lv_obj_t *m_page = nullptr;
};
