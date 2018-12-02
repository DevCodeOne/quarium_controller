#pragma once

#include <array>
#include <atomic>
#include <mutex>
#include <optional>
#include <thread>
#include <vector>

#include "lvgl.h"

#include "gui/manual_control_view.h"
#include "ring_buffer.h"

class main_view {
   public:
    static std::shared_ptr<main_view> instance();
    void open_view();
    void close_view();
    std::shared_ptr<manual_control_view> manual_control_view_instance();
    const std::shared_ptr<manual_control_view> manual_control_view_instance() const;

    ~main_view();

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

    main_view() = default;

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
    lv_obj_t *m_status_bar = nullptr;
    lv_obj_t *m_clock = nullptr;
    lv_theme_t *m_theme = nullptr;
    std::array<lv_obj_t *, 6> m_container;
    std::shared_ptr<manual_control_view> m_manual_view;
    std::string m_time;
    ring_buffer<page_index, 20> m_visited_pages;
    page_index m_current_page = page_index::front;
    lv_disp_drv_t m_display_driver;
    lv_indev_drv_t m_input_driver;

    static inline std::shared_ptr<main_view> _instance{nullptr};
    static inline std::recursive_mutex _instance_mutex{};

    static inline constexpr unsigned int screen_width = 320;
    static inline constexpr unsigned int screen_height = 480;
    static inline constexpr unsigned int navigation_buttons_height = 40;
    static inline constexpr unsigned int status_bar_height = 30;

    static inline constexpr std::array<const char [32], 6> front_button_titles = {"Schedules", "Manual Control", "Modules", "Stats", "Logs", ""};

    friend class view_controller;
};
