#pragma once

#include <map>

#include "pattern_templates/singleton.h"

template<typename K, typename V>
class value_storage {
   public:
    using Key_Type = K;
    using Value_Type = V;

    static std::shared_ptr<value_storage> instance();

    ~value_storage() = default;

    bool create_value(const K &key, const V &value);
    bool change_value(const K &key, const V &value);
    bool remove_value(const K &key);

    std::optional<V> retrieve_value(const K &key) const;

   private:
    value_storage() = default;

    std::map<K, V> m_values;

    friend class singleton<value_storage>;
};

template<typename K, typename V>
std::shared_ptr<value_storage<K, V>> value_storage<K, V>::instance() {
    return singleton<value_storage<K, V>>::instance();
}

template<typename K, typename V>
bool value_storage<K, V>::create_value(const K &key, const V &value) {
    return m_values.try_emplace(key, value).second;
}

template<typename K, typename V>
bool value_storage<K, V>::change_value(const K &key, const V &value) {
    m_values[key] = value;
    return true;
}

template<typename K, typename V>
bool value_storage<K, V>::remove_value(const K &key) {
    return m_values.erase(key);
}

template<typename K, typename V>
std::optional<V> value_storage<K, V>::retrieve_value(const K &key) const {
    if (auto result = m_values.find(key); result != m_values.cend()) {
        return {result->second};
    }

    return {};
}
