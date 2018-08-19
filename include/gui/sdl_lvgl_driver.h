#pragma once

#include <atomic>
#include <memory>
#include <mutex>
#include <algorithm>

#include "SDL.h"
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

    SDL_Window *m_window = nullptr;
    SDL_Surface *m_surface = nullptr;
    bool is_pressed = false;
    int m_mouse_x = 0, m_mouse_y = 0;
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

    int act_x1 = std::clamp<int>(x1, 0, inst->m_surface->w - 1);
    int act_y1 = std::clamp<int>(y1, 0, inst->m_surface->h - 1);
    int act_x2 = std::clamp<int>(x2, 0, inst->m_surface->w - 1);
    int act_y2 = std::clamp<int>(y2, 0, inst->m_surface->h - 1);

    SDL_LockSurface(inst->m_surface);

    T *pixels = (T *)inst->m_surface->pixels;

    int32_t line_length = inst->m_surface->pitch / sizeof(T);

    for (int32_t y_it = act_y1; y_it <= act_y2; ++y_it) {
        long off_y = y_it * line_length;
        for (int32_t x_it = act_x1; x_it <= act_x2; ++x_it) {
            pixels[x_it + off_y] = color_pointer->full;
            ++color_pointer;
        }
        color_pointer += (x2 - act_x2);
    }

    SDL_UnlockSurface(inst->m_surface);
}
