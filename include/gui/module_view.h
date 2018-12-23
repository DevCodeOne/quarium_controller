#pragma once

#include <map>
#include <memory>

#include "lvgl.h"

#include "gui/page_interface.h"
#include "modules/module_interface.h"
#include "value_storage.h"

struct module_value_element_storage {
    std::shared_ptr<module_interface> module;
    std::string m_value_id;
};

class single_module_value_element {
   public:
    single_module_value_element(lv_obj_t *parent, std::shared_ptr<module_interface> module, const module_value &value);

    lv_obj_t *container();

   private:
    lv_obj_t *m_value_manipulator = nullptr;
    lv_obj_t *m_value_name = nullptr;
    lv_obj_t *const m_container = nullptr;
    module_value m_value;
    const int m_value_storage_id = 0;

    static inline unsigned int _value_storage_id = 0;
};

class single_module_element {
   public:
    single_module_element(lv_obj_t *parent, std::shared_ptr<module_interface> module);

    lv_obj_t *container();

   private:
    lv_obj_t *const m_container = nullptr;
    std::vector<single_module_value_element> m_values;
    std::shared_ptr<module_interface> m_module;
};

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
    void create_gui();
    void switch_to_module(std::shared_ptr<module_interface> module);
    void create_module_page(std::shared_ptr<module_interface> module);

    lv_obj_t *m_container = nullptr;
    lv_obj_t *m_module_chooser = nullptr;
    std::map<std::string, single_module_element> m_module_views;

    friend class module_view_controller;
};
