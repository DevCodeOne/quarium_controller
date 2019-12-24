#include "io/outputs/remote_function/remote_function.h"

#include <charconv>
#include <string>

#include "httplib/httplib.h"
#include "logger.h"

// TODO: replace [] operator with find, otherwise if the key doesn't exist the program will fail
std::unique_ptr<output_interface> remote_function::create_for_interface(const nlohmann::json &description_parameter) {
    nlohmann::json description = description_parameter;
    auto logger_instance = logger::instance();
    if (!description.is_object()) {
        return nullptr;
    }

    auto url_entry = description["url"];

    if (url_entry.is_null() || !url_entry.is_string()) {
        logger_instance->info("The url_entry of one output is invalid");
        return {};
    }

    auto value_entry = description["value"];

    if (value_entry.is_null() || !value_entry.is_object()) {
        logger_instance->info("The value_entry of one output is invalid");
        return {};
    }

    auto value_id_entry = value_entry["name"];

    if (value_id_entry.is_null() || !value_id_entry.is_string()) {
        logger_instance->info("The value_id of a value of one output is invalid");
        return {};
    }

    auto created_output_value = output_value::deserialize(value_entry["description"]);

    if (!created_output_value.has_value()) {
        logger_instance->info("The parsed output_value description was invalid");
        return {};
    }

    return std::unique_ptr<output_interface>(
        new remote_function(url_entry.get<std::string>(), value_id_entry.get<std::string>(), *created_output_value));
}

remote_function::remote_function(const std::string &url, const std::string &value_id, const output_value &value)
    : m_url(url), m_value_id(value_id), m_value(value) {}

bool remote_function::remote_function::control_output(const output_value &value) {
    m_value = value;
    return update_values();
}

bool remote_function::override_with(const output_value &value) {
    m_overriden_value = value;
    return update_values();
}

bool remote_function::restore_control() {
    m_overriden_value = {};
    return update_values();
}

std::optional<output_value> remote_function::is_overriden() const {
    if (!m_overriden_value.has_value()) {
        return {};
    }
    return m_overriden_value.value();
}

output_value remote_function::current_state() const {
    if (m_overriden_value.has_value()) {
        return *m_overriden_value;
    }

    return m_value;
}

bool remote_function::update_values() {
    // Check if it is a valid url
    auto logger_instance = logger::instance();
    auto port_start = m_url.find_last_of(":");
    auto port_end = m_url.find_first_not_of("0123456789", port_start + 1);

    std::string host = m_url;
    std::string port = "80";
    std::string function = "";

    if (port_start != std::string::npos && port_end != std::string::npos) {
        host = m_url.substr(0, port_start);
        port = m_url.substr(port_start + 1, port_end - (port_start + 1));
        function = m_url.substr(port_end);
    } else {
        auto function_pos = m_url.find_last_of("/");

        if (function_pos == std::string::npos) {
            // TODO: check if the url is valid in the parsing process
            logger_instance->critical("Invalid url for remote function");
            return false;
        }

        host = m_url.substr(0, function_pos);
        function = m_url.substr(function_pos);
    }

    std::ostringstream remote_function("");

    remote_function << function;
    auto value = current_state();
    if (auto serialized_value = value.serialize<std::string>(); serialized_value) {
        remote_function << "?" << m_value_id << "=" << *serialized_value;
    } else {
        logger_instance->info("The value of this type couldn't be serialized");
    }

    uint16_t port_value = 0;
    auto remote_function_val = remote_function.str();

    if (std::from_chars(port.data(), port.data() + port.size(), port_value).ec != std::errc()) {
        logger_instance->critical("Couldn't partse port {}", port);
        return false;
    }

    httplib::Client request(host.c_str(), port_value, 5);
    logger_instance->info("Url to connect to {}:{}{}", host.c_str(), port_value, remote_function_val);

    auto request_result = request.Get(remote_function_val.c_str());

    if (!request_result) {
        logger_instance->critical("Couldn't connect to the host");
        return false;
    }

    if (request_result->status != 200) {
        logger_instance->warn("Request returned not ok status {}", request_result->status);
        return false;
    }

    return true;
}
