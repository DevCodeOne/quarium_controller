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

        inline gpiod_line<real>::gpiod_line(gpiod_chip<real> &chip, unsigned int offset) {
            gpiod_chip_get_line(chip.native(), offset);
        }

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

        inline void gpiod_line<real>::release_resource() { gpiod_line_release(m_native); }

        inline native_gpiod_line *gpiod_line<real>::native() { return m_native; }

        inline const native_gpiod_line *gpiod_line<real>::native() const { return m_native; }

    }  // namespace detail

    using gpiod_chip = detail::gpiod_chip<detail::type>;
    using gpiod_line = detail::gpiod_line<detail::type>;

}  // namespace gpiod
