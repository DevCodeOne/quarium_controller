#pragma once

#include "lvgl.h"

class view_controller {
   public:
    view_controller() = delete;

   private:
    static lv_res_t front_button_event(lv_obj_t *button);
    static lv_res_t navigation_event(lv_obj_t *button, const char *button_text);
    static lv_res_t select_gpio_event(lv_obj_t *ddlist);

    friend class view;
};
