#pragma once

#include <memory>
#include <mutex>

template<typename T>
class singleton {
   public:
    using mutex_type = std::recursive_mutex;

    static std::shared_ptr<T> instance();
    static std::lock_guard<mutex_type> retrieve_instance_lock();

   private:
    static inline std::shared_ptr<T> _instance{nullptr};
    static inline mutex_type _instance_mutex{};
};

template<typename T>
std::shared_ptr<T> singleton<T>::instance() {
    std::lock_guard<mutex_type> _instance_guard{_instance_mutex};

    if (_instance == nullptr) {
        _instance = std::shared_ptr<T>(new T);
    }

    return _instance;
}

template<typename T>
std::lock_guard<typename singleton<T>::mutex_type> singleton<T>::retrieve_instance_lock() {
    return std::lock_guard<mutex_type>{_instance_mutex};
}
