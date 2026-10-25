#pragma once

#include <format>
#include <print>
#include <sstream>
#include <source_location>
#include <type_traits>

namespace util {

struct Failure {};

namespace detail {

template <class... Args>
struct SrcMessageFormat {
    std::format_string<Args...> fmt_;
    std::source_location sl_;


    template <class T>
        requires std::convertible_to< const T&, std::format_string<Args...> >
    consteval SrcMessageFormat( const T& fmt, std::source_location sl = std::source_location::current()) noexcept
        : fmt_{ fmt }
        , sl_{ sl }
    {}
};

}

template <class... Args>
[[noreturn]] void fail(detail::SrcMessageFormat<std::type_identity_t<Args>...> fmt, Args&&... args) {
    std::println(
        stderr,
        "Failure at {}({},{}):{}", 
        fmt.sl_.file_name(), 
        fmt.sl_.line(), 
        fmt.sl_.column(),
        fmt.sl_.function_name()
    );
    std::println( stderr, fmt.fmt_, std::forward<Args>(args)... );
    throw Failure{};
}

}