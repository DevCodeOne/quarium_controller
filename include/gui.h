#pragma once

#include <atomic>
#include <mutex>
#include <optional>
#include <thread>

#include "GUIslice.h"
#include "GUIslice_drv.h"

class gui {
   public:
    static std::optional<std::shared_ptr<gui>> instance();
    void open_gui();
    void close_gui();

    ~gui();

   private:
    static void gui_loop();
    static void init_environment_variables();

    static bool quit(void*, void*, gslc_teTouch, short int, short int);

    gui() = default;

    static inline constexpr char framebuffer[] = "/dev/fb0";
    static inline constexpr char sdl_videodriver[] = GSLC_DEV_VID_DRV;

    static inline constexpr char tslib_device[] = "/dev/input/event0";
    static inline constexpr char tslib_calibration_file[] = "/etc/pointercal";
    static inline constexpr char tslib_configuration_file[] = "/etc/ts.conf";

    static inline constexpr int number_of_pages = 2;
    static inline constexpr int number_of_fonts = 2;
    static inline constexpr int number_of_elements = 2;

    static inline std::shared_ptr<gui> _instance;
    static inline std::recursive_mutex _instance_mutex;

    gslc_tsGui m_gui;
    gslc_tsDriver m_driver;
    gslc_tsPage m_pages[number_of_pages];
    gslc_tsFont m_fonts[number_of_fonts];
    gslc_tsElem m_page_elements[number_of_elements];
    gslc_tsElemRef m_page_element_refs[number_of_elements];
    std::thread m_gui_thread;
    std::atomic_bool m_should_exit = false;
    std::atomic_bool m_is_started = false;
    bool m_init = false;
};
