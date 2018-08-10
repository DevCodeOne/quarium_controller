#include "logger.h"

#include "lvgl_driver.h"

std::shared_ptr<lvgl_driver> lvgl_driver::instance() {
    std::lock_guard<std::mutex> instance_guard{_instance_mutex};

    if (!_instance) {
        _instance = std::shared_ptr<lvgl_driver>(new lvgl_driver);
    }

    return _instance;
}

lvgl_driver::lvgl_driver() {
    int tty_fd = open("/dev/tty0", O_RDONLY, 0);

    if (tty_fd < 0) {
        logger::instance()->warn("An error occured when trying to open /dev/tty0 {}", strerror(errno));
    }

    // if (ioctl(tty_fd, KDSETMODE, KD_TEXT) < 0) {
    //     logger::instance()->warn("An error occured when trying to set the mode of /dev/tty0 {}", strerror(errno));
    // }

    if (ioctl(tty_fd, KDSETMODE, KD_GRAPHICS) < 0) {
        logger::instance()->warn("An error occured when trying to set the mode of /dev/tty0 {}", strerror(errno));
    }

    // TODO Make configurable via settings.json
    m_framebuffer_descriptor = open("/dev/fb0", O_RDWR);

    if (m_framebuffer_descriptor < 0) {
        logger::instance()->warn("An error occured when trying to open the framebuffer /dev/fb0");
        m_is_valid = false;
        return;
    }

    if (ioctl(m_framebuffer_descriptor, FBIOGET_VSCREENINFO, &m_variable_info) < 0) {
        logger::instance()->warn("An error occured when trying to get the variable info of /dev/fb0");
        m_is_valid = false;
        return;
    }

    m_variable_info.bits_per_pixel = 16;

    if (ioctl(m_framebuffer_descriptor, FBIOPUT_VSCREENINFO, &m_variable_info) < 0) {
        logger::instance()->warn("An error occured when trying to set the variable info of /dev/fb0");
        m_is_valid = false;
        return;
    }

    if (ioctl(m_framebuffer_descriptor, FBIOGET_FSCREENINFO, &m_fixed_info) < 0) {
        logger::instance()->warn("An error occured when trying to get the fixed info of /dev/fb0");
        m_is_valid = false;
        return;
    }

    m_framebuffer_memory_length = m_variable_info.xres * m_variable_info.yres * m_variable_info.bits_per_pixel / 8;

    m_framebuffer_memory = (char *)mmap(nullptr, m_framebuffer_memory_length, PROT_READ | PROT_WRITE, MAP_SHARED,
                                        m_framebuffer_descriptor, 0);

    if ((void *)m_framebuffer_memory == MAP_FAILED) {
        logger::instance()->warn("Couldn't mmap framebuffer memory");
        m_is_valid = false;
        return;
    }

    // TODO Make configurable via settings.json
    setenv((char *)"TSLIB_FBDEVICE", "/dev/fb0", 1);
    setenv((char *)"TSLIB_TSDEVICE", "/dev/input/event0", 1);
    setenv((char *)"TSLIB_CALIBFILE", (char *)"/etc/pointercal", 1);
    setenv((char *)"TSLIB_CONFFILE", (char *)"/etc/ts.conf", 1);

    // TODO Make configurable via settings.json
    m_touch_device = ts_open("/dev/input/event0", 1);

    if (m_touch_device == nullptr) {
        logger::instance()->warn("An error occured when trying to open the touch device /dev/input/event0");
        m_is_valid = false;
        return;
    }

    if (ts_config(m_touch_device) < 0) {
        logger::instance()->warn("An error occured when trying to configure the touch device /dev/input/event0");
        m_is_valid = false;
        return;
    }
}

lvgl_driver::~lvgl_driver() {
    int tty_fd = open("/dev/tty0", O_RDONLY, 0);

    if (tty_fd < 0) {
        logger::instance()->warn("An error occured when trying to open /dev/tty0 {}", strerror(errno));
    }
    if (ioctl(tty_fd, KDSETMODE, KD_TEXT) < 0) {
        logger::instance()->warn("An error occured when trying to set the mode of /dev/tty0 {}", strerror(errno));
    }
}

lvgl_driver::operator bool() const { return m_is_valid; }

void lvgl_driver::flush_buffer(int32_t x1, int32_t y1, int32_t x2, int32_t y2, const lv_color_t *color_pointer) {
    auto inst = instance();

    if (inst == nullptr || inst->operator bool() == false) {
        logger::instance()->warn("lv driver is not valid");
        return;
    }

    if (inst->m_framebuffer_memory == nullptr || x2 < 0 || y2 < 0 || x1 > inst->m_variable_info.xres - 1 ||
        y1 > inst->m_variable_info.yres - 1) {
        lv_flush_ready();
        return;
    }

    switch (inst->m_variable_info.bits_per_pixel) {
        case 32:
        case 24:
            do_copy<uint32_t>(x1, y1, x2, y2, color_pointer);
            break;
        case 16:
            do_copy<uint16_t>(x1, y1, x2, y2, color_pointer);
            break;
        case 8:
            do_copy<uint8_t>(x1, y1, x2, y2, color_pointer);
            break;
        default:
            break;
    }

    lv_flush_ready();
}

bool lvgl_driver::handle_input(lv_indev_data_t *data) {
    auto inst = instance();

    ts_sample sample;

    ts_read(inst->m_touch_device, &sample, 1);
    data->point.x = sample.x;
    data->point.y = sample.y;
    data->state = sample.pressure > 10 ? LV_INDEV_STATE_PR : LV_INDEV_STATE_REL;
    return false;
}
