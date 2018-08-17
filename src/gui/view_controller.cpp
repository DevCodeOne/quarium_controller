#include <string_view>

#include "gui/view_controller.h"
#include "gui/view.h"

lv_res_t view_controller::front_button_event(lv_obj_t *obj) {
    auto inst = view::instance();

    if (!inst) {
        return LV_RES_OK;
    }

    lv_obj_t *label = lv_obj_get_child(obj, nullptr);

    if (!label) {
        return LV_RES_OK;
    }

    std::string_view text = lv_label_get_text(label);

    for (uint8_t i = 0; i <= (uint8_t)view::page_index::logs; ++i) {
        if (text == inst->front_button_titles[i]) {
            inst->switch_page(view::page_index(i));
            break;
        }
    }

    return LV_RES_OK;
}

lv_res_t view_controller::navigation_event(lv_obj_t *obj, const char *button_text) {
    auto inst = view::instance();

    if (!inst) {
        return LV_RES_OK;
    }

    std::string_view button_text_string = button_text;

    if (button_text_string == "Back") {
        inst->switch_to_last_page();
    } else if (button_text_string == "Home") {
        inst->switch_page(view::page_index::front);
    } else if (button_text_string == "Config") {
        inst->switch_page(view::page_index::configuration);
    }

    return LV_RES_OK;
}

lv_res_t view_controller::select_gpio_event(lv_obj_t *ddlist) {
    uint16_t index = lv_ddlist_get_selected(ddlist);

    return LV_RES_OK;
}


