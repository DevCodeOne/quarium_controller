#pragma once

#include <memory>
#include <vector>

#include "curl/curl.h"

#include "io/outputs/output_interface.h"
#include "io/outputs/output_value.h"

// TODO create own datatype so multiple values can be manipulated (with json objects)
class remote_function final : public output_interface {
   public:
    virtual ~remote_function() = default;

    static std::unique_ptr<output_interface> create_for_interface(const nlohmann::json &description);

    virtual bool control_output(const output_value &value) override;
    virtual bool override_with(const output_value &value) override;
    virtual bool restore_control() override;
    virtual std::optional<output_value> is_overriden() const override;
    virtual output_value current_state() const override;

   private:
    remote_function(const std::string &url, const std::string &value_id, const output_value &value);

    bool update_values();

    output_value m_value;
    std::optional<output_value> m_overriden_value;
    std::unique_ptr<CURL, decltype(curl_easy_cleanup)*> m_curl_instance{nullptr, curl_easy_cleanup};
    const std::string m_value_id;
    const std::string m_url;
};
