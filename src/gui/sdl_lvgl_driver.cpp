#include "posix_includes.h"

#include "gui/sdl_lvgl_driver.h"

std::shared_ptr<lvgl_driver> lvgl_driver::instance() {
    std::lock_guard<std::mutex> instance_guard{_instance_mutex};

    if (!_instance) {
        _instance = std::shared_ptr<lvgl_driver>(new lvgl_driver);
    }

    return _instance;
}

lvgl_driver::lvgl_driver() {
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        m_is_valid = false;
    }

    m_window = SDL_CreateWindow("quarium_controller", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, LV_HOR_RES,
                                LV_VER_RES, SDL_WINDOW_SHOWN);
    m_surface = SDL_CreateRGBSurfaceWithFormat(0, LV_HOR_RES, LV_VER_RES, LV_COLOR_DEPTH, SDL_PIXELFORMAT_RGB565);

    if (!m_window || !m_surface) {
        m_is_valid = false;
    }
}

lvgl_driver::~lvgl_driver() {
    SDL_DestroyWindow(m_window);
    SDL_FreeSurface(m_surface);
    SDL_Quit();
}

void lvgl_driver::flush_buffer(int32_t x1, int32_t y1, int32_t x2, int32_t y2, const lv_color_t *color_pointer) {
    auto inst = instance();

    if (inst == nullptr || inst->operator bool() == false) {
        lv_flush_ready();
        return;
    }

    if ((inst->m_window == nullptr && inst->m_surface) || x2 < 0 || y2 < 0 || x1 > inst->m_surface->w - 1 ||
        y1 > inst->m_surface->h - 1) {
        lv_flush_ready();
        return;
    }

    switch (inst->m_surface->format->BitsPerPixel) {
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

    SDL_BlitSurface(inst->m_surface, nullptr, SDL_GetWindowSurface(inst->m_window), nullptr);
    SDL_UpdateWindowSurface(inst->m_window);

    lv_flush_ready();
}

lvgl_driver::operator bool() const { return m_is_valid; }

bool lvgl_driver::handle_input(lv_indev_data_t *data) {
    auto inst = instance();

    if (inst == nullptr || inst->operator bool() == false) {
        return false;
    }

    SDL_Event event;

    while (SDL_PollEvent(&event) != 0) {
        if (event.type == SDL_QUIT) {
            kill(getpid(), SIGINT);
        }

        if (event.type == SDL_MOUSEMOTION || event.type == SDL_MOUSEBUTTONDOWN || event.type == SDL_MOUSEBUTTONUP) {
            inst->is_pressed = SDL_BUTTON(SDL_BUTTON_LEFT) & SDL_GetMouseState(&inst->m_mouse_x, &inst->m_mouse_y);
        }
    }

    data->point.x = inst->m_mouse_x;
    data->point.y = inst->m_mouse_y;
    data->state = inst->is_pressed ? LV_INDEV_STATE_PR : LV_INDEV_STATE_REL;

    return false;
}
