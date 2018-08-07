#include "GUIslice.h"

#include "gui.h"
#include "logger.h"
#include "signal_handler.h"

bool gui::quit(void*, void*, gslc_teTouch, short int, short int) {
    logger::instance()->warn("Quiting");
    return true;
}

// TODO use settings to configure those variables and use these values as default
void gui::init_environment_variables() {
    setenv("FRAMEBUFFER", framebuffer, 1);
    setenv("SDL_FBDEV", framebuffer, 1);
    setenv("SDL_VIDEODRIVER", sdl_videodriver, 1);

    setenv("TSLIB_FBDEVICE", framebuffer, 1);
    setenv("TSLIB_TSDEVICE", tslib_device, 1);
    setenv("TSLIB_CALIBFILE", tslib_calibration_file, 1);
    setenv("TSLIB_CONFFILE", tslib_configuration_file, 1);
}

std::optional<std::shared_ptr<gui>> gui::instance() {
    std::lock_guard<std::recursive_mutex> _instance_guard(_instance_mutex);

    if (_instance) {
        return _instance;
    }

    _instance = std::shared_ptr<gui>(new gui);
    return _instance;
}

void gui::open_gui() {
    std::lock_guard<std::recursive_mutex> _instance_guard(_instance_mutex);
    if (m_is_started) {
        return;
    }

    m_gui_thread = std::thread(gui_loop);
}

void gui::close_gui() {
    std::lock_guard<std::recursive_mutex> _instance_guard(_instance_mutex);
    if (!m_is_started) {
        return;
    }

    m_should_exit = true;
    m_gui_thread.join();
    m_is_started = false;
}

void gui::gui_loop() {
    signal_handler::disable_for_current_thread();

    std::lock_guard<std::recursive_mutex> _instance_guard(_instance_mutex);
    auto instance_opt = instance();

    if (!instance_opt) {
        return;
    }

    auto inst = instance_opt.value();

    init_environment_variables();

    if (!gslc_Init(&inst->m_gui, &inst->m_driver, inst->m_pages, number_of_pages, inst->m_fonts, number_of_fonts)) {
        logger::instance()->critical("Couldn't init gui");
        return;
    }

    inst->m_init = true;

    if (bool result = gslc_FontAdd(&inst->m_gui, 0, GSLC_FONTREF_FNAME, "/usr/share/fonts/TTF/DejaVuSans.ttf", 10);
        !result) {
        logger::instance()->critical("Couldn't add font");
    }

    gslc_PageAdd(&inst->m_gui, 0, inst->m_page_elements, number_of_pages, inst->m_page_element_refs, number_of_pages);
    gslc_SetBkgndColor(&inst->m_gui, GSLC_COL_GRAY_DK2);

    gslc_tsElemRef *elem_ref = nullptr;

    while (!inst->m_should_exit) {
        gslc_Update(&inst->m_gui);
    }

    elem_ref = gslc_ElemCreateBox(&inst->m_gui, 0, 0, (gslc_tsRect){10, 50, 300, 150});
    gslc_ElemSetCol(&inst->m_gui, elem_ref, GSLC_COL_WHITE, GSLC_COL_BLACK, GSLC_COL_BLACK);

    char text[] = "Quit";

    elem_ref = gslc_ElemCreateBtnTxt(&inst->m_gui, 1, 0, (gslc_tsRect){120, 100, 80, 40}, text, 0,
                                     0, &gui::quit);
}

gui::~gui() {
    if (!m_init) {
        return;
    }
    gslc_Quit(&m_gui);
}
