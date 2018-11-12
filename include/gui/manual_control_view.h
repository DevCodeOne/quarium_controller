#pragma once

#include <memory>

#include "lvgl.h"

class manual_control_view {
   public:
    manual_control_view(lv_obj_t *container);
    void update_contents();

    lv_obj_t *container();
    lv_obj_t *gpio_chooser();
    lv_obj_t *gpio_control_switch();
    lv_obj_t *gpio_override_checkbox();

    const lv_obj_t *container() const;
    const lv_obj_t *gpio_chooser() const;
    const lv_obj_t *gpio_control_switch() const;
    const lv_obj_t *gpio_override_checkbox() const;

    size_t gpio_list_string_len() const;

   private:
    void create_gui();

    lv_obj_t *m_container = nullptr;
    lv_obj_t *m_gpio_chooser = nullptr;
    lv_obj_t *m_gpio_control_switch = nullptr;
    lv_obj_t *m_gpio_override_checkbox = nullptr;
    std::unique_ptr<char[]> m_gpio_list = nullptr;
    size_t m_gpio_list_len = 0;
};
