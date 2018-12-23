#pragma once

#include "lvgl.h"

class single_module_value_element_controller {
   public:
    single_module_value_element_controller() = delete;

   private:
    static lv_res_t value_changed(lv_obj_t *slider);

    friend class single_module_value_element;
};

class module_view_controller {
   public:
    module_view_controller() = delete;

   private:
    static lv_res_t module_selected(lv_obj_t *ddlist);

    friend class module_view;
};
