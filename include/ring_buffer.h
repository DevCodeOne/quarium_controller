#pragma once

#include <algorithm>
#include <array>
#include <initializer_list>
#include <optional>

template<typename T, size_t N>
class ring_buffer {
   public:
    using size_type = decltype(N);

    ring_buffer(const std::initializer_list<T> &init = {});

    void put(const T &value);
    void remove_last_element();

    constexpr std::optional<T> at(size_type pos) const;
    std::optional<T> retrieve_last_element() const;
    size_type size() const;
    constexpr size_type capacity() const;

   private:
    std::array<T, N> m_buffer;
    size_type m_start = 0;
    size_type m_end = 0;
    size_type m_number_of_elements = 0;
};

template<typename T, size_t N>
ring_buffer<T, N>::ring_buffer(const std::initializer_list<T> &init) {
    std::copy(init.begin(), init.end(), m_buffer.begin());
    m_end = init.size() > 0 ? init.size() - 1 : 0;
    m_number_of_elements = init.size();
}

template<typename T, size_t N>
void ring_buffer<T, N>::put(const T &value) {
    m_number_of_elements = std::min(m_number_of_elements + 1, N);
    if (m_number_of_elements != 1) {
        m_end = (m_end + 1) % N;
    }

    m_buffer[m_end] = value;

    if (m_end == m_start && m_number_of_elements != 1) {
        m_start = (m_start + 1) % N;
    }
}

template<typename T, size_t N>
void ring_buffer<T, N>::remove_last_element() {
    if (m_number_of_elements == 0) {
        return;
    }

    m_end = (m_end > 0 ? m_end : N) - 1;
    --m_number_of_elements;
}

template<typename T, size_t N>
constexpr std::optional<T> ring_buffer<T, N>::at(size_type pos) const {
    if (pos >= m_number_of_elements) {
        return {};
    }

    return m_buffer[(m_start + pos) % N];
}

template<typename T, size_t N>
std::optional<T> ring_buffer<T, N>::retrieve_last_element() const {
    if (m_number_of_elements == 0) {
        return {};
    }

    return at(m_number_of_elements - 1);
}

template<typename T, size_t N>
auto ring_buffer<T, N>::size() const -> size_type {
    return m_number_of_elements;
}

template<typename T, size_t N>
constexpr auto ring_buffer<T, N>::capacity() const -> size_type {
    return N;
}
