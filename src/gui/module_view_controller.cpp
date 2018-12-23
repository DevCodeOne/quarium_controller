#include "gui/module_view_controller.h"
#include "gui/main_view.h"
#include "logger.h"
#include "modules/module_collection.h"

lv_res_t single_module_value_element_controller::value_changed(lv_obj_t *slider) {
    auto values = value_storage<unsigned int, module_value_element_storage>::instance();

    auto value = values->retrieve_value(lv_obj_get_free_num(slider));

    if (!value.has_value()) {
        return LV_RES_OK;
    }

    if (value->module == nullptr) {
        return LV_RES_OK;
    }

    value->module->description().set_value<int>(value->m_value_id, lv_slider_get_value(slider));
    value->module->update_values();

    return LV_RES_OK;
}

// TODO : fix issue here maximum string length for
lv_res_t module_view_controller::module_selected(lv_obj_t *ddlist) {
    char buf[512];

    lv_ddlist_get_selected_str(ddlist, buf);

    auto module_collection_instance = module_collection::instance();
    if (!module_collection_instance) {
        return LV_RES_OK;
    }

    auto module_instance = module_collection_instance->get_module(buf);
    if (!module_instance) {
        return LV_RES_OK;
    }

    auto main_view_instance = main_view::instance();
    if (!main_view_instance) {
        return LV_RES_OK;
    }

    auto page_view = main_view_instance->view_instance(main_view::page_index::module);
    if (!page_view) {
        return LV_RES_OK;
    }

    std::shared_ptr<module_view> module_view_instance = std::dynamic_pointer_cast<module_view>(page_view);
    if (!module_view_instance) {
        return LV_RES_OK;
    }

    module_view_instance->switch_to_module(module_instance);

    return LV_RES_OK;
}
