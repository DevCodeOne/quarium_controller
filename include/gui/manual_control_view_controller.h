#pragma once

#include "lvgl.h"

class manual_control_view_controller {
   public:
    manual_control_view_controller() = delete;

   private:
    static lv_res_t toggle_switch(lv_obj_t *sw);
    static lv_res_t check_override_schedule(lv_obj_t *checkbox);

    friend class manual_control_view;
};
