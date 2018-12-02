#pragma once

#if __has_include(<experimental/filesystem>)
#include <experimental/filesystem>
namespace fs = std::experimental::filesystem;
#else
#include <filesystem>
namespace fs = std::filesystem;
#endif

class aws_iot_slot_config {
   public:
    aws_iot_slot_config() = default;
    ~aws_iot_slot_config() = default;

    aws_iot_slot_config &certificate_path(const fs::path &path);
    aws_iot_slot_config &key_path(const fs::path &path);

    const fs::path &certificate_path() const;
    const fs::path &key_path() const;

   private:
    fs::path m_certificate_path;
    fs::path m_key_path;
};

class aws_iot_slot {
   public:
   private:
};

class aws_iot_interface {
   public:
   private:
};
