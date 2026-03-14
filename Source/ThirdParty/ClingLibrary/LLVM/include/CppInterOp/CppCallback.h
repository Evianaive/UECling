#ifndef CPPINTEROP_CPPCALLBACK_H
#define CPPINTEROP_CPPCALLBACK_H

#include <cstddef>
#include <utility>
#include <type_traits>
#include <cassert>

namespace CppImpl {

template<typename Signature>
class CppCallback;

template<typename R, typename... Args>
class CppCallback<R(Args...)> {
    using Invoker_t = R (*)(void*, Args...);

    struct IManagedData {
        virtual ~IManagedData() = default;
        virtual IManagedData* Clone() const = 0;
        virtual void* GetAddress() = 0;
    };

    template<typename T>
    struct ManagedDataImpl final : IManagedData {
        T m_Callable;
        template<typename U>
        ManagedDataImpl(U&& u) : m_Callable(std::forward<U>(u)) {}
        IManagedData* Clone() const override { return new ManagedDataImpl(m_Callable); }
        void* GetAddress() override { return &m_Callable; }
    };

    Invoker_t m_Invoker = nullptr;
    IManagedData* m_Data = nullptr;

    template<typename T>
    static R InvokeInternal(void* data, Args... args) {
        return (*static_cast<T*>(data))(std::forward<Args>(args)...);
    }

public:
    CppCallback() = default;

    CppCallback(std::nullptr_t) : m_Invoker(nullptr), m_Data(nullptr) {}

    template<typename T, typename = std::enable_if_t<!std::is_same_v<std::decay_t<T>, CppCallback>>>
    CppCallback(T&& callable) {
        using DecayedT = std::decay_t<T>;
        m_Data = new ManagedDataImpl<DecayedT>(std::forward<T>(callable));
        m_Invoker = &InvokeInternal<DecayedT>;
    }

    ~CppCallback() {
        delete m_Data;
    }

    CppCallback(const CppCallback& other)
        : m_Invoker(other.m_Invoker), m_Data(other.m_Data ? other.m_Data->Clone() : nullptr) {}

    CppCallback& operator=(const CppCallback& other) {
        if (this != &other) {
            delete m_Data;
            m_Invoker = other.m_Invoker;
            m_Data = other.m_Data ? other.m_Data->Clone() : nullptr;
        }
        return *this;
    }

    CppCallback(CppCallback&& other) noexcept 
        : m_Invoker(other.m_Invoker), m_Data(other.m_Data) {
        other.m_Invoker = nullptr;
        other.m_Data = nullptr;
    }

    CppCallback& operator=(CppCallback&& other) noexcept {
        if (this != &other) {
            delete m_Data;
            m_Invoker = other.m_Invoker;
            m_Data = other.m_Data;
            other.m_Invoker = nullptr;
            other.m_Data = nullptr;
        }
        return *this;
    }

    explicit operator bool() const { return m_Invoker != nullptr; }

    R operator()(Args... args) const {
        assert(m_Invoker && "Attempting to call an unbound CppCallback!");
        return m_Invoker(m_Data->GetAddress(), std::forward<Args>(args)...);
    }
};

} // namespace CppImpl

#endif // CPPINTEROP_CPPCALLBACK_H
