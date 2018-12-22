#pragma once

#include "lvgl.h"

class module_view_controller {
   public:
    module_view_controller() = delete;

   private:
    static lv_res_t module_selected(lv_obj_t *ddlist);

    friend class module_view;
};
