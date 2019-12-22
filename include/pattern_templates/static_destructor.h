#pragma once

#include <atomic>

template<typename Clazz, typename Callable>
class static_desctructor final {
   public:
    using clazz = Clazz;
    using callable = Callable;

    inline static_desctructor(Callable destructor) : m_destructor(destructor) { _instances.fetch_add(1); }

    inline ~static_desctructor() {
        if (!(_instances.fetch_sub(1) - 1)) {
            m_destructor();
        }
    }

   private:
    Callable m_destructor;
    static inline std::atomic_uint32_t _instances{0};
};
