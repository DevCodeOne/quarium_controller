#include <cstdint>
#include <filesystem>
#include <memory>

#include "io/outputs/can/can.h"
#include "io/outputs/output_interface.h"
#include "io/outputs/output_value.h"
#include "value_transitioner.h"

class can_output final : public output_interface {
   public:
    can_output(const can_output &output) = delete;
    can_output(can_output &&output) = delete;
    virtual ~can_output() = default;

    can_output &operator=(const can_output &) = delete;
    can_output &operator=(can_output &&) = delete;

    virtual bool control_output(const output_value &value) override;
    virtual bool override_with(const output_value &value) override;
    virtual bool restore_control() override;
    virtual std::optional<output_value> is_overriden() const override;
    virtual output_value current_state() const override;

    static std::unique_ptr<can_output> create_for_interface(const nlohmann::json &description);

   private:
    template<typename Callable>
    can_output(std::shared_ptr<can> can_instance, can_object_identifier identifier, const output_value &initial_value,
               Callable transition);

    can_error_code update_value();
    can_error_code sync_values();

    output_value m_value;
    value_transitioner<output_value> m_transitioner;
    std::optional<output_value> m_overriden_value{};
    can_object_identifier m_object_identifier;
    std::shared_ptr<can> m_can_instance;
};
