#include "io/outputs/mqtt/mqtt_output.h"

#include "io/outputs/output_transition.h"
#include "logger.h"

std::unique_ptr<mqtt_output> mqtt_output::create_for_interface(const nlohmann::json &description) {
    using namespace std::literals;

    auto logger_instance = logger::instance();
    if (!description.is_object()) {
        logger_instance->critical("Description of this output is not an object");
        return nullptr;
    }

    auto mqtt_url_entry = description.find("url");
    auto mqtt_port_entry = description.find("port");
    auto mqtt_topic_entry = description.find("topic");
    auto mqtt_default_value = description.find("default");
    auto mqtt_username_entry = description.find("user");
    auto mqtt_password_entry = description.find("password");

    if (mqtt_url_entry == description.cend() || !mqtt_url_entry->is_string()) {
        logger_instance->critical("The url entry of the event is not valid, description : {}", description.dump());
        return nullptr;
    }

    if (mqtt_port_entry == description.cend() || !mqtt_port_entry->is_number_unsigned()) {
        logger_instance->critical("The port entry of the event is not valid, description : {}", description.dump());
        return nullptr;
    }

    if (mqtt_topic_entry == description.cend() || !mqtt_topic_entry->is_string()) {
        logger_instance->critical("The topic entry of the event is not valid, description : {}", description.dump());
        return nullptr;
    }

    if (mqtt_username_entry == description.cend() ^ mqtt_password_entry == description.cend()) {
        logger_instance->critical("The mqtt credentials are only partially specialized");
        return nullptr;
    }

    auto url = mqtt_url_entry->get<std::string>();
    auto port = mqtt_port_entry->get<uint16_t>();
    auto topic = mqtt_topic_entry->get<std::string>();
    auto default_value = (mqtt_default_value != description.cend() && mqtt_default_value->is_string())
                             ? mqtt_default_value->get<std::string>()
                             : ""s;
    bool use_credentials = false;
    std::string user = "";
    std::string password = "";

    if ((mqtt_username_entry != description.cend() && mqtt_username_entry->is_string()) &&
        (mqtt_password_entry != description.cend() && mqtt_password_entry->is_string())) {
        use_credentials = true;
        user = mqtt_username_entry->get<std::string>();
        password = mqtt_password_entry->get<std::string>();
    }

    mqtt_address addr{"", .m_port = mqtt_port{port}};
    std::strncpy(addr.m_server_name, url.c_str(), mqtt_address::max_server_name_len);

    auto mqtt_instance = mqtt::instance(addr, use_credentials ? std::optional<mqtt_credentials>(mqtt_credentials{
                                                                    .m_password = password, .m_username = user})
                                                              : std::optional<mqtt_credentials>());

    if (!mqtt_instance) {
        logger_instance->critical("No valid mqtt interface could be aquired");
        return nullptr;
    }

    return std::unique_ptr<mqtt_output>(new mqtt_output(mqtt_instance, mqtt_topic{topic.c_str()},
                                                        output_value{default_value}, output_transitions::instant<>{}));
}

bool mqtt_output::sync_values() { return update_value(); }

bool mqtt_output::update_value() {
    auto logger_instance = logger::instance();
    if (!m_mqtt_instance) {
        logger_instance->critical("No valid mqtt_instance for this instance is available");
        return false;
    }

    std::string value_to_send = "";
    output_value value = current_state();

    switch (value.current_type()) {
        case output_value_types::string:
            value_to_send = *value.get<std::string>();
            break;
        default:
            logger_instance->error("Tried to send an invalid data type via mqtt");
            return false;
    }

    auto result = m_mqtt_instance->publish(m_topic, value_to_send);

    // TODO: request data, so that one can confirm that the data has actually been written, is that even necessary with
    // mqtt ?
    if (!result) {
        logger_instance->warn("Couldn't send data via mqtt");
    }

    return result;
}

bool mqtt_output::control_output(const output_value &value) {
    logger::instance()->info("Control value : {}", *value.get<std::string>());
    m_value = value;
    m_transitioner.target_value(value);

    return sync_values();
}

bool mqtt_output::override_with(const output_value &value) {
    m_overriden_value = value;
    m_transitioner.target_value(value);

    return sync_values();
}

bool mqtt_output::restore_control() {
    m_overriden_value.reset();
    m_transitioner.target_value(m_value);

    return sync_values();
}

std::optional<output_value> mqtt_output::is_overriden() const { return m_overriden_value; }

output_value mqtt_output::current_state() const { return m_transitioner.current_value(); }

