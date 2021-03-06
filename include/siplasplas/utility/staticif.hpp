#ifndef SIPLASPLAS_UTILITY_STATICIF_HPP
#define SIPLASPLAS_UTILITY_STATICIF_HPP

#include "meta.hpp"
#include "identity.hpp"
#include <utility>

namespace cpp
{

namespace detail
{

/**
 * \brief Implements the then branch of an static conditional.
 *
 * The cpp::detail::If template implements the internals of an `static_if()`.
 * The main template implements the true condition branch (The branch executing the then
 * body). Returning values in this case may not be as efficient as possible since the returned
 * value should bypass the then body (This involves two move operations, see cpp::If::ElseBypass):
 *
 * ``` cpp
 * std::string str = If<true>().Then([](auto identity)
 * {
 *     return "hello, world!"s; // the string is moved twice
 *                              // to exit the static if.
 * });
 * ```
 *
 * \tparam Condition If condition. True in the main template.
 */
template<bool Condition>
class If
{
public:
    template<typename T>
    class ElseBypass
    {
    public:
        constexpr ElseBypass(T&& value) :
            _value{std::move(value)}
        {}

        template<typename Body>
        constexpr T Else(const Body&)
        {
            return std::move(_value);
        }

    private:
        T _value;
    };

    template<typename T>
    class ElseBypass<T&>
    {
    public:
        constexpr ElseBypass(T& value) :
            _value{&value}
        {}

        template<typename Body>
        constexpr T& Else(const Body&)
        {
            return static_cast<T&>(*this);
        }

        constexpr operator T&()
        {
            return *_value;
        }

        T& operator()()
        {
            return *_value;
        }

    private:
        T* _value;
    };

    template<typename Body, typename... Args>
    constexpr auto Then(const Body& body, Args&&... args) ->
        typename std::enable_if<
            !std::is_void<decltype(body(::cpp::Identity(), std::forward<Args>(args)...))>::value,
            ElseBypass<decltype(body(::cpp::Identity(), std::forward<Args>(args)...))>
        >::type
    {
        return body(::cpp::Identity(), std::forward<Args>(args)...);
    }

    template<typename Body, typename... Args>
    constexpr auto Then(const Body& body, Args&&... args) ->
        typename std::enable_if<
            std::is_void<decltype(body(::cpp::Identity(), std::forward<Args>(args)...))>::value,
            If&
        >::type
    {
        body(::cpp::Identity(), std::forward<Args>(args)...);
        return *this;
    }


    template<typename Body, typename... Args>
    constexpr void Else(const Body&, Args&&...)
    {
        /* NOTHING */
    }
};

template<>
class If<false>
{
public:
    constexpr If() = default;

    template<typename Body, typename... Args>
    constexpr If& Then(const Body&, Args&&...)
    {
        return *this;
    }

    template<typename Body, typename... Args>
    constexpr decltype(auto) Else(const Body& body, Args&&... args)
    {
        return body(Identity(), std::forward<Args>(args)...);
    }
};

}

/**
 * \ingroup utility
 * \brief Implements an static conditional
 *
 * An static conditional allows to conditionally
 * evaluate some code depending on the value of a compile
 * time property. The body of the conditional is implemented
 * by user provided functions.
 *
 * ``` cpp
 * template<typename T>
 * void f(T value)
 * {
 *     cpp::staticIf<std::is_integral<T>::value>([&](auto identity)
 *     {
 *         std::cout << "the value is integral\n";
 *     });
 *
 *     ...
 * }
 * ```
 *
 * The body of the untaken conditional path shall not be
 * evaluated by the compiler and can contain ill formed code.
 * **NOTE**: This behavior relies on the two phase template processing
 * scheme, so the statement above is only true for entities that
 * will be evaluated in the instantiation phase only:
 *
 * ``` cpp
 * template<typename T>
 * void foo(T value)
 * {
 *    int i = 0;
 *
 *    // The condition is false, so the code
 *    // inside the if "should" not be evaluated:
 *    cpp::staticIf<false>([&](auto identity)
 *    {
 *        // Ok: value depends on T template parameter.
 *        value.TheMostBizarreMethodName();
 *
 *        // ERROR: 'int' type is not class/struct/union.
 *        i.method();
 *
 *        // Ok: identity(i) depends on a template parameter.
 *        identity(i).method();
 *    });
 * }
 * ```
 *
 * As the example shows, the conditional body takes an `identity`
 * parameter that can be used to force the evaluation of an expression
 * at the second phase.
 *
 * The static conditional expression also provides an `else` sentence
 * in the form of an `Else()` method:
 *
 * ``` cpp
 * template<typename T>
 * void foo(T value)
 * {
 *    int i = 0;
 *
 *    cpp::staticIf<false>([&](auto identity)
 *    {
 *        value.TheMostBizarreMethodName();
 *        i.method();
 *        identity(i).method();
 *    }).Else([&](auto identity)
 *    {
 *        std::cout << i << std::endl;
 *    });
 * }
 * ```
 *
 * cpp::staticIf() supports returning values from the conditional
 * body too:
 *
 * ``` cpp
 * template<typename T>
 * T twice(const T& value)
 * {
 *     return cpp::staticIf<std::is_class<T>::value>([&](auto)
 *     {
 *         return value.twice();
 *     }).Else([&](auto)
 *     {
 *         return value * 2;
 *     });
 * }
 * ```
 *
 * note this has some caveats:
 *
 *  - **Return types cannot be inferred**: Due to technical
 *    reasons `auto` or any other kind of type inference cannot
 *    be used with the return value of a static conditional.
 *
 *  - **Return may not be optimal in some code paths**: If the condition
 *    is true, the `then` path is picked. If the `then` body returns a value,
 *    such value is not returned directly (So elligible for RVO) but bypassed
 *    through the `else` internals. This means **returning a value from a
 *    positive conditional may involve two move operations**.
 *
 * \tparam Condition Value of the condition. The value should be evaluable
 * at compile time, else compilation fails.
 * \tparam ThenBody Function type with one template-dependent value parameter.
 * \param thenBody Body of the `then` path of the conditional. Evaluated
 * only if the condition is true.
 * \return An unspecified type implementing the `else` part of the
 * conditional.
 */
template<bool Condition, typename ThenBody, typename... Args>
auto staticIf(const ThenBody& thenBody, Args&&... args)
{
    detail::If<Condition> if_;
    return if_.Then(thenBody, std::forward<Args>(args)...);
}

}

#endif // SIPLASPLAS_UTILITY_STATICIF_HPP
