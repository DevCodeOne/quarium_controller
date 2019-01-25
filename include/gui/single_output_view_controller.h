#pragma once

#include "lvgl.h"

#include "io/outputs/output_value.h"

class single_output_view_controller {
   public:
    single_output_view_controller() = delete;

   private:
    static lv_res_t override_checkbox_action(lv_obj_t *override_checkbox);
    static lv_res_t override_value_action(lv_obj_t *override_value);

    static bool set_value_to_current_state(lv_obj_t *override_value, const output_value &value);

    friend class single_output_view;
};
