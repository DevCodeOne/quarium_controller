#pragma once

#include <array>
#include <atomic>
#include <mutex>
#include <optional>
#include <thread>
#include <vector>

#include "lvgl.h"

#include "ring_buffer.h"

class view {
   public:
    static std::shared_ptr<view> instance();
    void open_view();
    void close_view();

    ~view();

   private:
    enum struct page_index : uint8_t {
        schedule_list = 0,
        manual_control = 1,
        stats = 2,
        logs = 3,
        configuration = 4,
        front = 5,
    };

    static void view_loop();

    view() = default;

    void create_pages();
    void switch_page(const page_index &new_index);
    void update_contents(const page_index &index);
    void switch_to_last_page();

    std::thread m_view_thread;
    std::atomic_bool m_should_exit{false};
    std::atomic_bool m_is_started{false};
    bool m_is_initialized = false;

    lv_obj_t *m_screen = nullptr;
    lv_obj_t *m_content_container = nullptr;
    lv_theme_t *m_theme = nullptr;
    std::array<lv_obj_t *, 6> m_container;
    ring_buffer<page_index, 20> m_visited_pages;
    page_index current_page = page_index::front;
    lv_disp_drv_t m_display_driver;
    lv_indev_drv_t m_input_driver;

    lv_obj_t *m_gpio_chooser = nullptr;
    std::unique_ptr<char[]> m_gpio_list = nullptr;

    static inline std::shared_ptr<view> _instance{nullptr};
    static inline std::recursive_mutex _instance_mutex{};

    static inline constexpr unsigned int screen_width = 320;
    static inline constexpr unsigned int screen_height = 480;
    static inline constexpr unsigned int navigation_buttons_height = 40;

    static inline constexpr char front_button_titles[5][15] = {"Schedules", "Manual Control", "Stats", "Logs", ""};

    friend class view_controller;
};
