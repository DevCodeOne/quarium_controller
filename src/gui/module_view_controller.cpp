#include "gui/module_view_controller.h"
#include "gui/main_view.h"
#include "logger.h"
#include "modules/module_collection.h"

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
