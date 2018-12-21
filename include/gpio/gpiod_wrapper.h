#pragma once

#include "gpiod.h"

#include <map>
#include <mutex>

namespace gpiod {

    using native_gpiod_chip = ::gpiod_chip;
    using native_gpiod_line = ::gpiod_line;

    namespace detail {

        struct stub {};
        struct real {};
#ifdef USE_GPIOD_STUBS
        using type = stub;
#else
        using type = real;
#endif

        template<typename T>
        class gpiod_chip;

        template<typename T>
        class gpiod_line;

        template<>
        class gpiod_line<real> {
           public:
            gpiod_line(const gpiod_line<real> &other) = delete;
            gpiod_line(gpiod_line<real> &&other);
            ~gpiod_line();

            gpiod_line &operator=(const gpiod_line<real> &other) = delete;
            gpiod_line &operator=(gpiod_line<real> &&other);
            void swap(gpiod_line<real> &other);

            int get_value();
            int set_value(int value);
            int request_output_flags(const char *consumer, int flags, int default_val);

            explicit operator bool() const;

            void release_resource();

           private:
            gpiod_line(gpiod_chip<real> &chip, unsigned int offset);

            native_gpiod_line *native();
            const native_gpiod_line *native() const;

            native_gpiod_line *m_native = nullptr;

            friend class gpiod_chip<real>;
        };

        template<>
        class gpiod_chip<real> {
           public:
            gpiod_chip(const char *path);
            gpiod_chip(const gpiod_chip<real> &other) = delete;
            gpiod_chip(gpiod_chip<real> &&other);
            ~gpiod_chip();

            gpiod_chip &operator=(const gpiod_chip<real> &other) = delete;
            gpiod_chip &operator=(gpiod_chip<real> &&other);
            void swap(gpiod_chip<real> &other);

            explicit operator bool() const;

            gpiod_line<real> get_line(unsigned int offset);

            void release_resource();

           private:
            native_gpiod_chip *native();
            const native_gpiod_chip *native() const;

            native_gpiod_chip *m_native = nullptr;
            friend class gpiod_line<real>;
        };

        template<>
        class gpiod_line<stub> {
           public:
            gpiod_line(const gpiod_line<stub> &other) = delete;
            gpiod_line(gpiod_line<stub> &&other);
            ~gpiod_line() = default;

            gpiod_line &operator=(const gpiod_line<stub> &other) = delete;
            gpiod_line &operator=(gpiod_line<stub> &&other);
            void swap(gpiod_line<stub> &other);

            int get_value();
            int set_value(int value);
            int request_output_flags(const char *consumer, int flags, int default_val);

            explicit operator bool() const;

            void release_resource();

           private:
            gpiod_line(gpiod_chip<stub> &chip, unsigned int offset);

            int m_offset = 0;
            int m_value = 0;
            int m_flags = 0;
            bool m_valid = false;
            std::string m_consumer = "";

            friend class gpiod_chip<stub>;
        };

        template<>
        class gpiod_chip<stub> {
           public:
            gpiod_chip(const char *path);
            gpiod_chip(const gpiod_chip<stub> &other) = delete;
            gpiod_chip(gpiod_chip<stub> &&other) = default;
            ~gpiod_chip() = default;

            gpiod_chip &operator=(const gpiod_chip<stub> &other) = delete;
            gpiod_chip &operator=(gpiod_chip<stub> &&other) = default;
            void swap(gpiod_chip<stub> &other);

            explicit operator bool() const;

            gpiod_line<stub> get_line(unsigned int offset);

            void release_resource();

           private:
            std::string m_path;

            friend class gpiod_line<stub>;
        };

        inline gpiod_chip<real>::gpiod_chip(const char *path) : m_native(gpiod_chip_open(path)) {}

        inline gpiod_chip<real>::gpiod_chip(gpiod_chip<real> &&other) : m_native(other.m_native) {
            other.m_native = nullptr;
        }
        inline gpiod_chip<real>::~gpiod_chip() { release_resource(); }

        inline gpiod_chip<real> &gpiod_chip<real>::operator=(gpiod_chip<real> &&other) {
            gpiod_chip<real> tmp(std::move(other));
            swap(tmp);

            return *this;
        }

        inline void gpiod_chip<real>::swap(gpiod_chip<real> &other) {
            using std::swap;
            swap(m_native, other.m_native);
        }

        inline gpiod_chip<real>::operator bool() const { return m_native != nullptr; }

        inline gpiod_line<real> gpiod_chip<real>::get_line(unsigned int offset) {
            return gpiod_line<real>(*this, offset);
        }

        inline void gpiod_chip<real>::release_resource() {
            if (!m_native) {
                return;
            }

            gpiod_chip_close(m_native);
            m_native = nullptr;
        }

        inline native_gpiod_chip *gpiod_chip<real>::native() { return m_native; }

        inline const native_gpiod_chip *gpiod_chip<real>::native() const { return m_native; }

        inline gpiod_line<real>::gpiod_line(gpiod_chip<real> &chip, unsigned int offset)
            : m_native(gpiod_chip_get_line(chip.native(), offset)) {}

        inline gpiod_line<real>::gpiod_line(gpiod_line<real> &&other) : m_native(other.m_native) {
            other.m_native = nullptr;
        }

        inline gpiod_line<real>::~gpiod_line() { release_resource(); }

        inline gpiod_line<real> &gpiod_line<real>::operator=(gpiod_line<real> &&other) {
            gpiod_line<real> tmp(std::move(other));
            swap(tmp);

            return *this;
        }

        inline void gpiod_line<real>::swap(gpiod_line<real> &other) {
            using std::swap;

            swap(m_native, other.m_native);
        }

        inline int gpiod_line<real>::get_value() { return gpiod_line_get_value(native()); }

        inline int gpiod_line<real>::set_value(int value) { return gpiod_line_set_value(native(), value); }

        inline int gpiod_line<real>::request_output_flags(const char *consumer, int flags, int default_val) {
            return gpiod_line_request_output_flags(native(), consumer, flags, default_val);
        }

        inline gpiod_line<real>::operator bool() const { return m_native != nullptr; }

        inline void gpiod_line<real>::release_resource() {
            if (!m_native) {
                return;
            }

            gpiod_line_release(m_native);
            m_native = nullptr;
        }

        inline native_gpiod_line *gpiod_line<real>::native() { return m_native; }

        inline const native_gpiod_line *gpiod_line<real>::native() const { return m_native; }

        inline gpiod_chip<stub>::gpiod_chip(const char *path) : m_path(path) {}

        inline void gpiod_chip<stub>::swap(gpiod_chip<stub> &other) {
            using std::swap;

            swap(m_path, other.m_path);
        }

        inline gpiod_chip<stub>::operator bool() const { return m_path != ""; }

        inline gpiod_line<stub> gpiod_chip<stub>::get_line(unsigned int offset) {
            return gpiod_line<stub>(*this, offset);
        }

        inline void gpiod_chip<stub>::release_resource() {}

        inline gpiod_line<stub>::gpiod_line(gpiod_chip<stub> &chip, unsigned int offset)
            : m_offset(offset), m_value(0), m_valid(true) {}

        inline gpiod_line<stub>::gpiod_line(gpiod_line<stub> &&other)
            : m_offset(other.m_offset),
              m_value(other.m_value),
              m_flags(other.m_flags),
              m_valid(other.m_valid),
              m_consumer(std::move(other.m_consumer)) {
            other.m_offset = 0;
            other.m_value = 0;
            other.m_flags = 0;
            other.m_valid = false;
            other.m_consumer = "";
        }

        inline gpiod_line<stub> &gpiod_line<stub>::operator=(gpiod_line<stub> &&other) {
            gpiod_line<stub> tmp(std::move(other));
            swap(tmp);

            return *this;
        }

        inline void gpiod_line<stub>::swap(gpiod_line<stub> &other) {
            using std::swap;

            swap(m_offset, other.m_offset);
            swap(m_value, other.m_value);
            swap(m_flags, other.m_flags);
            swap(m_valid, other.m_valid);
            swap(m_consumer, other.m_consumer);
        }

        inline int gpiod_line<stub>::get_value() { return m_value; }

        inline int gpiod_line<stub>::set_value(int value) {
            m_value = value;
            return 0;
        }

        inline int gpiod_line<stub>::request_output_flags(const char *consumer, int flags, int default_val) {
            m_consumer = consumer;
            m_flags = flags;
            m_value = default_val;
            return 0;
        }

        inline gpiod_line<stub>::operator bool() const { return m_valid; }

        inline void gpiod_line<stub>::release_resource() { m_valid = false; }

    }  // namespace detail

    using gpiod_chip = detail::gpiod_chip<detail::type>;
    using gpiod_line = detail::gpiod_line<detail::type>;

}  // namespace gpiod
