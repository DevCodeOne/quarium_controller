#pragma once

#include <atomic>
#include <mutex>
#include <optional>
#include <thread>

#include "lvgl.h"

class gui {
   public:
    static std::shared_ptr<gui> instance();
    void open_gui();
    void close_gui();

    ~gui();

   private:
    static void gui_loop();

    gui() = default;

    std::thread m_gui_thread;
    std::atomic_bool m_should_exit{false};
    std::atomic_bool m_is_started{false};
    bool m_is_initialized = false;

    lv_obj_t *m_screen = nullptr;
    lv_theme_t *m_theme = nullptr;
    lv_disp_drv_t m_display_driver;
    lv_indev_drv_t m_input_driver;

    // TODO Fix : Valgrind is complaining about use after free here
    static inline std::shared_ptr<gui> _instance{nullptr};
    static inline std::recursive_mutex _instance_mutex{};
};
