#include <charconv>
#include <regex>

#include "config.h"
#include "gpio/gpio_chip.h"
#include "logger.h"

std::shared_ptr<gpio_chip> gpio_chip::instance(const std::filesystem::path &gpio_chip_path) {
    std::lock_guard<std::recursive_mutex> _access_map_guard{_instance_mutex};

    if (auto result = _gpiochip_access_map.find(gpio_chip_path); result != _gpiochip_access_map.cend()) {
        return result->second;
    }

    if (!reserve_gpiochip(gpio_chip_path)) {
        return nullptr;
    }

    return _gpiochip_access_map[gpio_chip_path];
}

http::response<http::dynamic_body> gpio_chip::handle_request(const http::request<http::dynamic_body> &request) {
    http::response<http::dynamic_body> response;

    // TODO create utility for this standard behaviour
    std::string resource_path_string(request.target().to_string());
    std::regex list_gpio_chips_regex(R"(/api/v0/gpio_chip/?)", std::regex_constants::extended);
    std::regex gpio_chip_id_regex(R"([1-9][0-9]*|[0-9])", std::regex_constants::ECMAScript);

    if (std::regex_match(resource_path_string, list_gpio_chips_regex) && request.method() == http::verb::get) {
        std::lock_guard<std::recursive_mutex> instance_guard{_instance_mutex};

        nlohmann::json gpio_chips;

        auto add_serialized_gpio_chip = [&gpio_chips](const auto &current_gpio_chip) {
            nlohmann::json pair;
            pair["id"] = current_gpio_chip.second->id().as_number();
            pair["path"] = current_gpio_chip.second->path_to_file().c_str();
            gpio_chips.push_back(std::move(pair));
        };

        std::for_each(_gpiochip_access_map.cbegin(), _gpiochip_access_map.cend(), add_serialized_gpio_chip);

        boost::beast::ostream(response.body()) << gpio_chips.dump();
        return std::move(response);
    }

    std::smatch match;
    std::regex_search(resource_path_string, match, list_gpio_chips_regex);
    std::string id = match.suffix();

    if (std::regex_match(id, gpio_chip_id_regex)) {
        std::lock_guard<std::recursive_mutex> instance_guard{_instance_mutex};

        uint32_t id_as_number;
        auto conversion_result = std::from_chars<uint32_t>(id.data(), id.data() + id.size(), id_as_number);

        if (conversion_result.ec == std::errc::result_out_of_range) {
            boost::beast::ostream(response.body()) << "ID is not a number";
            return std::move(response);
        }

        nlohmann::json specific_gpio_chip;

        auto result = std::find_if(
            _gpiochip_access_map.cbegin(), _gpiochip_access_map.cend(),
            [id_as_number](const auto &current_chip) { return current_chip.second->id().as_number() == id_as_number; });

        if (result != _gpiochip_access_map.cend()) {
            boost::beast::ostream(response.body()) << result->second->serialize().dump();
            return std::move(response);
        }
    }

    boost::beast::ostream(response.body()) << "Couldn't find resource " << resource_path_string;
    return std::move(response);
}

std::optional<gpio_chip> gpio_chip::open(const std::filesystem::path &gpio_chip_path) {
    auto opened_chip = gpiod::gpiod_chip(gpio_chip_path.c_str());

    if (!opened_chip) {
        logger::instance()->critical("Couldn't open gpiochip {}", gpio_chip_path.c_str());
        return {};
    }

    return gpio_chip(gpio_chip_path, std::move(opened_chip));
}

bool gpio_chip::reserve_gpiochip(const std::filesystem::path &gpio_chip_path) {
    std::lock_guard<std::recursive_mutex> _access_map_guard{_instance_mutex};

    if (_gpiochip_access_map.find(gpio_chip_path) != _gpiochip_access_map.cend()) {
        return false;
    }

    if (auto created_gpio_chip = gpio_chip::open(gpio_chip_path); created_gpio_chip.has_value()) {
        _gpiochip_access_map.try_emplace(gpio_chip_path, std::make_shared<gpio_chip>(std::move(*created_gpio_chip)));
        return true;
    }

    return false;
}

bool gpio_chip::free_gpiochip(const std::filesystem::path &gpio_chip_path) {
    std::lock_guard<std::recursive_mutex> _access_map_guard{_instance_mutex};

    return _gpiochip_access_map.erase(gpio_chip_path);
}

gpio_chip::gpio_chip(const std::filesystem::path &gpio_dev, gpiod::gpiod_chip chip)
    : m_gpiochip_path(gpio_dev), m_chip(std::move(chip)) {}

gpio_chip::gpio_chip(gpio_chip &&other)
    : m_gpiochip_path(std::move(other.m_gpiochip_path)),
      m_chip(std::move(other.m_chip)),
      m_reserved_pins(std::move(other.m_reserved_pins)) {
    other.m_gpiochip_path = "";
}

gpio_chip::~gpio_chip() {
    if (m_chip) {
        // Possible problem in libgpiod where the chip might include the pins as resources, so the pins have to be
        // destroyed before the chip
        m_reserved_pins.clear();
        m_chip.release_resource();
    }
}

const std::filesystem::path &gpio_chip::path_to_file() const { return m_gpiochip_path; }

std::shared_ptr<gpio_pin> gpio_chip::open_pin(const gpio_pin_id &id) {
    if (auto result = m_reserved_pins.find(id); result != m_reserved_pins.cend()) {
        return result->second;
    }

    auto pin = gpio_pin::open(id);
    if (!pin.has_value()) {
        return nullptr;
    }

    // TODO FIX : Error is here remove all the code which adds elements to m_reserved_pins and the error is gone
    auto [position, result] = m_reserved_pins.try_emplace(id, std::make_shared<gpio_pin>(std::move(*pin)));
    return position->second;
}

nlohmann::json gpio_chip::serialize() const {
    nlohmann::json serialized;
    serialized["gpio_chip_path"] = m_gpiochip_path.c_str();

    for (const auto &[id, current_pin] : m_reserved_pins) {
        serialized["pins"].push_back(current_pin->serialize());
    }

    return std::move(serialized);
}
