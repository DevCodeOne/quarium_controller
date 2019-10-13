#include <filesystem>

#include "io/outputs/can/can.h"

class can_id {
   public:
    can_id(unsigned int id, std::shared_ptr<can> can_instance);

    unsigned int id() const;
    const std::filesystem::path &can_interface_path() const;

   private:
};

class can_output final : public output_interface {
   public:
    can_output(const can_output &output) = delete;
    can_output(can_output &&output) = default;
    virtual ~can_output() = default;

    can_output &operator=(const can_output &) = delete;
    can_output &operator=(can_output &) = default;

    virtual bool control_output(const output_value &value) override;
    virtual bool override_with(const output_value &value) override;
    virtual bool restore_control() override;
    virtual std::optional<output_value> is_overriden() const override;
    virtual output_value current_state() const override;

   private:
    output_value m_controlled_value;
    std::optional<output_value> m_overriden_value;
    can_id m_can_id;
};
