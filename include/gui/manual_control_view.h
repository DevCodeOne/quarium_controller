#pragma once

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "lvgl.h"

#include "gui/page_interface.h"

class manual_control_view final : public page_interface {
   public:
    manual_control_view(lv_obj_t *container);

    page_event enter_page() override;
    page_event leave_page() override;
    page_event update_contents() override;

    lv_obj_t *container();

    const lv_obj_t *container() const;

   private:
    // TODO put everything in one container so it can be easier aligned
    class output_override_element {
       public:
        output_override_element(lv_obj_t *parent, const std::string &id,
                                const std::string &override_text = "Placeholder Text");
        output_override_element(const output_override_element &) = delete;
        output_override_element(output_override_element &&) = default;
        ~output_override_element() = default;
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
    std::vector<output_override_element> m_manual_overrides;
    lv_obj_t *m_page = nullptr;

    friend class manual_control_view_controller;
};
