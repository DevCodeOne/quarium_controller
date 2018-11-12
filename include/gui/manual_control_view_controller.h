#pragma once

#include "lvgl.h"

class manual_control_view_controller {
   public:
    manual_control_view_controller() = delete;

   private:
    static lv_res_t select_gpio_event(lv_obj_t *ddlist);
    static lv_res_t manually_control_gpio(lv_obj_t *sw);
    static lv_res_t check_override_schedule(lv_obj_t *checkbox);

    friend class manual_control_view;
};
