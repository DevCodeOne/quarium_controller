#pragma once

#include "gpiod.h"

namespace detail {

    struct stub {};
    struct real {};

#ifdef USE_GPIOD_WRAPPER
    using type = stub;
#else
    using type = real;
#endif

    using native_gpiod_chip = gpiod_chip;
    using native_gpiod_line = gpiod_line;

    template<typename T>
    class gpiod;

    template<>
    class gpiod<real> {
       public:
        class gpiod_chip {
           public:
            gpiod_chip(native_gpiod_chip *native = nullptr);
            gpiod_chip(const gpiod_chip &other) = delete;
            gpiod_chip(gpiod_chip &&other);
            ~gpiod_chip();

            gpiod_chip &operator=(const gpiod_chip &other) = delete;
            gpiod_chip &operator=(gpiod_chip &&other);
            void swap(gpiod_chip &other);

            explicit operator bool() const;

            native_gpiod_chip *native();
            const native_gpiod_chip *native() const;
            void close_native();

           private:
            native_gpiod_chip *m_native = nullptr;
        };

        class gpiod_line {
           public:
            gpiod_line(native_gpiod_line *native = nullptr);
            gpiod_line(const gpiod_line &other) = delete;
            gpiod_line(gpiod_line &&other);
            ~gpiod_line();

            gpiod_line &operator=(const gpiod_line &other) = delete;
            gpiod_line &operator=(gpiod_line &&other);
            void swap(gpiod_line &other);

            explicit operator bool() const;

            native_gpiod_line *native();
            const native_gpiod_line *native() const;
            void close_native();

           private:
            native_gpiod_line *m_native = nullptr;
        };

        static gpiod_chip chip_open(const char *path);
        static void chip_close(gpiod_chip &chip);
        static void line_release(gpiod_line &line);
        static gpiod_line chip_get_line(gpiod_chip &chip, unsigned int offset);
        static int line_request_output_flags(gpiod_line &line, const char *consumer, int flags, int default_val);
        static int line_get_value(gpiod_line &line);
        static int line_set_value(gpiod_line &line, int value);
    };

    template<>
    class gpiod<stub> {
       public:
        class gpiod_chip {
           public:
           private:
        };

        class gpiod_line {
           public:
           private:
        };

        static gpiod_chip *chip_open(const char *path);
        static void chip_close(gpiod_chip *chip);
        static void line_release(gpiod_line *line);
        static gpiod_line *chip_get_line(gpiod_chip *chip, unsigned int offset);
        static int line_request_output_flags(gpiod_line *line, const char *consumer, int flags, int default_val);
        static int line_get_value(gpiod_line *line);
        static int line_set_value(gpiod_line *line, int value);
    };

    inline gpiod<real>::gpiod_chip::gpiod_chip(native_gpiod_chip *native) : m_native(native) {}

    inline gpiod<real>::gpiod_chip::gpiod_chip(gpiod_chip &&other) : m_native(other.m_native) {
        other.m_native = nullptr;
    }

    inline gpiod<real>::gpiod_chip::~gpiod_chip() { close_native(); }

    inline gpiod<real>::gpiod_chip &gpiod<real>::gpiod_chip::operator=(gpiod_chip &&other) {
        gpiod_chip tmp(std::move(other));

        swap(tmp);
        return *this;
    }

    inline void gpiod<real>::gpiod_chip::swap(gpiod_chip &other) {
        using std::swap;

        swap(m_native, other.m_native);
    }

    inline gpiod<real>::gpiod_chip::operator bool() const { return m_native != nullptr; }

    inline native_gpiod_chip *gpiod<real>::gpiod_chip::native() { return m_native; }

    inline const native_gpiod_chip *gpiod<real>::gpiod_chip::native() const { return m_native; }

    inline void gpiod<real>::gpiod_chip::close_native() {
        if (m_native) {
            gpiod_chip_close(m_native);
        }
    }

    inline gpiod<real>::gpiod_line::gpiod_line(native_gpiod_line *native) : m_native(native) {}

    inline gpiod<real>::gpiod_line::gpiod_line(gpiod_line &&other) : m_native(other.m_native) {
        other.m_native = nullptr;
    }

    inline gpiod<real>::gpiod_line::~gpiod_line() { close_native(); }

    inline gpiod<real>::gpiod_line &gpiod<real>::gpiod_line::operator=(gpiod_line &&other) {
        gpiod_line tmp(std::move(other));

        swap(tmp);
        return *this;
    }

    inline void gpiod<real>::gpiod_line::swap(gpiod_line &other) {
        using std::swap;

        swap(m_native, other.m_native);
    }

    inline gpiod<real>::gpiod_line::operator bool() const { return m_native != nullptr; }

    inline native_gpiod_line *gpiod<real>::gpiod_line::native() { return m_native; }

    inline const native_gpiod_line *gpiod<real>::gpiod_line::native() const { return m_native; }

    inline void gpiod<real>::gpiod_line::close_native() {
        if (m_native) {
            gpiod_line_release(m_native);
        }
    }

    inline gpiod<real>::gpiod_chip gpiod<real>::chip_open(const char *path) { return gpiod_chip_open(path); }

    inline void gpiod<real>::chip_close(gpiod_chip &chip) { chip.close_native(); }

    inline void gpiod<real>::line_release(gpiod_line &line) { line.close_native(); }

    inline gpiod<real>::gpiod_line gpiod<real>::chip_get_line(gpiod_chip &chip, unsigned int offset) {
        return gpiod_chip_get_line(chip.native(), offset);
    }

    inline int gpiod<real>::line_request_output_flags(gpiod_line &line, const char *consumer, int flags,
                                                      int default_val) {
        return gpiod_line_request_output_flags(line.native(), consumer, flags, default_val);
    }

    inline int gpiod<real>::line_get_value(gpiod_line &line) { return gpiod_line_get_value(line.native()); }

    inline int gpiod<real>::line_set_value(gpiod_line &line, int value) {
        return gpiod_line_set_value(line.native(), value);
    }

}  // namespace detail

using gpiod = detail::gpiod<detail::type>;
