#pragma once

#include <atomic>
#include <mutex>
#include <optional>
#include <thread>

class gui {
   public:
    static std::optional<std::shared_ptr<gui>> instance();
    void open_gui();
    void close_gui();

    ~gui();

   private:
    static void gui_loop();
    static void init_environment_variables();

    gui() = default;

    static inline constexpr char framebuffer[] = "/dev/fb0";

    static inline constexpr char tslib_device[] = "/dev/input/event0";
    static inline constexpr char tslib_calibration_file[] = "/etc/pointercal";
    static inline constexpr char tslib_configuration_file[] = "/etc/ts.conf";

    static inline constexpr int number_of_pages = 2;
    static inline constexpr int number_of_fonts = 2;
    static inline constexpr int number_of_elements = 2;

    static inline std::shared_ptr<gui> _instance;
    static inline std::recursive_mutex _instance_mutex;

    std::thread m_gui_thread;
    std::atomic_bool m_should_exit{false};
    std::atomic_bool m_is_started{false};
    bool m_init = false;
};
