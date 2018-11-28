#pragma once

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "lvgl.h"

class manual_control_view {
   public:
    manual_control_view(lv_obj_t *container);
    void update_contents();

    lv_obj_t *container();

    const lv_obj_t *container() const;

   private:
    class gpio_override_element {
       public:
        gpio_override_element(lv_obj_t *parent, const std::string &id,
                              const std::string &override_text = "Placeholder Text");
        gpio_override_element(const gpio_override_element &) = delete;
        gpio_override_element(gpio_override_element &&) = default;
        ~gpio_override_element() = default;
        void text(const std::string &data);

        lv_obj_t *override_checkbox();
        lv_obj_t *override_switch();

        const char *text() const;
        const lv_obj_t *override_checkbox() const;
        const lv_obj_t *override_switch() const;
        const std::string &id() const;

       private:
        const std::string m_id;
        std::unique_ptr<char[]> m_override_text;
        lv_obj_t *m_override_checkbox;
        lv_obj_t *m_override_switch;
    };

    void create_gui();
    void recreate_override_elements();
    void update_override_elements();

    lv_obj_t *m_container = nullptr;
    std::vector<gpio_override_element> m_manual_overrides;
    lv_obj_t *m_page = nullptr;

    friend class manual_control_view_controller;
};
