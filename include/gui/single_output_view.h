#pragma once

#include <atomic>
#include <string>

#include "lvgl.h"

#include "value_storage.h"

struct single_output_element_storage final {
   public:
    std::string m_output_id;
    lv_obj_t *const m_container;
    lv_obj_t *const m_override_checkbox;
    lv_obj_t *const m_override_value;
};

class single_output_view final {
   public:
    using value_storage_type = value_storage<int, single_output_element_storage>;

    single_output_view(lv_obj_t *parent, const std::string &output_id);
    single_output_view(const single_output_view &) = delete;
    single_output_view(single_output_view &&) = default;
    ~single_output_view() = default;

    lv_obj_t *container();
    void update_gui();

   private:
    void create_gui(lv_obj_t *parent);

    std::unique_ptr<lv_style_t> m_container_style = nullptr;
    lv_obj_t *const m_container = nullptr;
    lv_obj_t *m_override_checkbox = nullptr;
    lv_obj_t *m_override_value = nullptr;

    const std::string m_output_id;
    const int m_value_storage_id = 0;

    static inline constexpr const char override_text[] = "Ctrl ";
    static inline std::atomic_int id = 0;
};
