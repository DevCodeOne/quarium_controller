#pragma once

#include <map>
#include <memory>

#include "lvgl.h"

#include "gui/page_interface.h"
#include "modules/module_interface.h"

// TODO seperate single module view
class module_view final : public page_interface {
   public:
    module_view(lv_obj_t *container);
    ~module_view() = default;

    page_event enter_page() override;
    page_event leave_page() override;
    page_event update_contents() override;

    lv_obj_t *container();

    const lv_obj_t *container() const;

   private:
    class module_single_value_element {
       public:
        module_single_value_element(lv_obj_t *container, const module_value &value);

        lv_obj_t *container();
        void set_callback_on_change(lv_res_t (*)(lv_obj_t *obj));

       private:
        lv_obj_t *m_value_manipulator = nullptr;
        lv_obj_t *m_value_name = nullptr;
        lv_obj_t *const m_container = nullptr;
        module_value m_value;

        static inline constexpr float percent_per_step = 0.1f;
    };

    void create_gui();
    void switch_to_module(std::shared_ptr<module_interface> module);
    void create_module_page(std::shared_ptr<module_interface> module);

    lv_obj_t *m_container = nullptr;
    lv_obj_t *m_module_chooser = nullptr;
    std::map<std::string, lv_obj_t *> m_pages;
    std::map<std::string, std::vector<module_single_value_element>> m_values;

    friend class module_view_controller;
};
