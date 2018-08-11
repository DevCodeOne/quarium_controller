#pragma once

#include <array>
#include <atomic>
#include <mutex>
#include <optional>
#include <thread>
#include <vector>

#include "lvgl.h"

class gui {
   public:
    static std::shared_ptr<gui> instance();
    void open_gui();
    void close_gui();

    ~gui();

   private:
    enum struct page_index : uint8_t {
        schedule_list = 0,
        manual_control = 1,
        stats = 2,
        logs = 3,
        configuration = 4,
        front = 5,
    };

    static void gui_loop();
    static lv_res_t front_button_event(lv_obj_t *button);
    static lv_res_t navigation_event(lv_obj_t *button, const char *button_text);

    gui() = default;

    void create_pages();
    void switch_page(const page_index &new_index);
    void switch_to_last_page();

    std::thread m_gui_thread;
    std::atomic_bool m_should_exit{false};
    std::atomic_bool m_is_started{false};
    bool m_is_initialized = false;

    lv_obj_t *m_screen = nullptr;
    lv_obj_t *m_content_container = nullptr;
    lv_theme_t *m_theme = nullptr;
    std::array<lv_obj_t *, 6> m_container;
    // Replace with ring buffer
    std::vector<page_index> m_visited_pages;
    page_index current_page = page_index::front;
    lv_disp_drv_t m_display_driver;
    lv_indev_drv_t m_input_driver;

    static inline std::shared_ptr<gui> _instance{nullptr};
    static inline std::recursive_mutex _instance_mutex{};

    static inline constexpr unsigned int screen_width = 320;
    static inline constexpr unsigned int screen_height = 480;
    static inline constexpr unsigned int navigation_buttons_height = 40;

    static inline constexpr char front_button_titles[5][15] = {"Schedules", "Manual Control", "Stats", "Logs", ""};
};
