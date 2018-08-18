#pragma once

#include "lvgl.h"

class view_controller {
   public:
    view_controller() = delete;

   private:
    static lv_res_t front_button_event(lv_obj_t *button);
    static lv_res_t navigation_event(lv_obj_t *button, const char *button_text);
    static lv_res_t select_gpio_event(lv_obj_t *ddlist);
    static lv_res_t check_override_schedule(lv_obj_t *checkbox);
    static lv_res_t manually_control_gpio(lv_obj_t *sw);

    friend class view;
};
