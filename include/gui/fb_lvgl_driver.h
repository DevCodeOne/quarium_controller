#pragma once

// clang-format off
#include "posix_includes.h"
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <sys/kd.h>
#include <sys/vt.h>
#include <linux/fb.h>
// clang-format on

#include <atomic>
#include <memory>
#include <mutex>
#include <chrono>
#include <algorithm>

#include "tslib.h"
#include "lvgl.h"

#include "logger.h"

class lvgl_driver {
   public:
    static std::shared_ptr<lvgl_driver> instance();
    static void flush_buffer(int32_t x1, int32_t y1, int32_t x2, int32_t y2, const lv_color_t *color_pointer);
    static bool handle_input(lv_indev_data_t *data);

    ~lvgl_driver();

    explicit operator bool() const;

   private:
    template<typename T>
    static void do_copy(int32_t x1, int32_t y1, int32_t x2, int32_t y2, const lv_color_t *color_pointer);

    lvgl_driver();

    fb_var_screeninfo m_variable_info;
    fb_fix_screeninfo m_fixed_info;
    int m_framebuffer_descriptor = 0;
    uint64_t m_framebuffer_memory_length = 0;
    char *m_framebuffer_memory = nullptr;
    tsdev *m_touch_device = nullptr;
    std::atomic_bool m_is_valid = true;

    static inline std::shared_ptr<lvgl_driver> _instance;
    static inline std::mutex _instance_mutex;
};

template<typename T>
void lvgl_driver::do_copy(int32_t x1, int32_t y1, int32_t x2, int32_t y2, const lv_color_t *color_pointer) {
    auto inst = instance();

    if (inst == nullptr || inst->operator bool() == false) {
        logger::instance()->warn("lv driver is not valid");
        return;
    }

    int act_x1 = std::clamp<int>(x1, 0, inst->m_variable_info.xres - 1);
    int act_y1 = std::clamp<int>(y1, 0, inst->m_variable_info.yres - 1);
    int act_x2 = std::clamp<int>(x2, 0, inst->m_variable_info.xres - 1);
    int act_y2 = std::clamp<int>(y2, 0, inst->m_variable_info.yres - 1);

    T *pixels = (T *)inst->m_framebuffer_memory;

    int32_t line_length = inst->m_fixed_info.line_length / sizeof(T);

    for (int32_t y_it = act_y1; y_it <= act_y2; ++y_it) {
        long off_y = y_it * line_length;
        for (int32_t x_it = act_x1; x_it <= act_x2; ++x_it) {
            pixels[(x_it + inst->m_variable_info.xoffset) + off_y] = color_pointer->full;
            ++color_pointer;
        }
        color_pointer += (x2 - act_x2);
    }
}
