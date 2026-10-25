#include <slop/slop.hpp>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <array>
#include <vector>
#include <limits>
#include <tuple>
#include <format>

static constexpr double PRECISE_EPS = std::numeric_limits<double>::epsilon();
static constexpr double NaN = std::numeric_limits<double>::quiet_NaN();
static constexpr double INF = std::numeric_limits<double>::infinity();

#define TEST_FUNCTION_GRADS( name_, func_, ... ) \
    TEST ( BasicFunction, Test ## name_ ) { \
        static constexpr auto data = [] { \
            std::vector< std::pair< \
            std::vector<double>, \
            std::vector<double> \
            > > inputExpected = __VA_ARGS__; \
            return std::make_tuple( \
                inputExpected.size(), \
                inputExpected[0].first.size() \
            ); \
        }(); \
        static constexpr auto arity = std::get<1>(data); \
        std::vector< std::pair < \
            std::array<double, arity>, \
            std::array<double, arity> \
        > > inputExpected = __VA_ARGS__; \
        for ( const auto& [ input, expected ] : inputExpected ) { \
            auto msg = std::format("Input of {}", input); \
            SCOPED_TRACE(msg); \
            auto t = slop::Tape<double>{}; \
            auto vars = std::apply([&] ( const auto&... values ) { return std::array{ t.Var(values)... }; }, input); \
            auto res = std::apply( [&] ( const auto&... values ) { return func_(values...); }, vars ); \
            t.Backward(res); \
            auto grads = std::apply([&] ( const auto&... vars ) { return std::array{ vars.Grad()... }; }, vars); \
            EXPECT_THAT( grads, ::testing::Pointwise( ::testing::NanSensitiveDoubleNear(PRECISE_EPS), expected )); \
        } \
    }

#define TEST_FUNCTION_GRADS_MATCH( name_, func_, expectedFunc_, ... ) \
    TEST ( BasicFunction, Test ## name_ ) { \
        static constexpr auto data = [] { \
            std::vector< std::vector<double> > inputs = __VA_ARGS__; \
            return std::make_tuple( \
                inputs.size(), \
                inputs[0].size() \
            ); \
        }(); \
        static constexpr auto arity = std::get<1>(data); \
        std::vector< std::array<double, arity> > inputs = __VA_ARGS__; \
        for ( const auto& input : inputs ) { \
            auto msg = std::format("Input of {}", input); \
            SCOPED_TRACE(msg); \
            auto t = slop::Tape<double>{}; \
            auto vars = std::apply([&] ( const auto&... values ) { return std::array{ t.Var(values)... }; }, input); \
            auto res = std::apply( [&] ( const auto&... values ) { return func_(values...); }, vars ); \
            t.Backward(res); \
            auto grads = std::apply([&] ( const auto&... vars ) { return std::array{ vars.Grad()... }; }, vars); \
            auto expected = std::apply([&] (const auto&... values ) { return expectedFunc_( values... ); }, input ); \
            EXPECT_THAT( grads, ::testing::Pointwise( ::testing::NanSensitiveDoubleNear(PRECISE_EPS), expected )); \
        } \
    }


TEST_FUNCTION_GRADS( Plus, []( auto a, auto b ) { return a + b; }, {
    { {{ 3.0,  6.0 }}, {{ 1.0, 1.0 }} },
    { {{ 9.0, -2.0 }}, {{ 1.0, 1.0 }} },
    { {{ 8.0, 2.54 }}, {{ 1.0, 1.0 }} },
    { {{ INF,  NaN }}, {{ 1.0, 1.0 }} } 
})

TEST_FUNCTION_GRADS( Multiply, []( auto a, auto b ) { return a * b; }, {
    { {{ 3.0,  6.0 }}, {{ 6.0, 3.0 }} },
    { {{ 9.0, -2.0 }}, {{ -2.0, 9.0 }} },
    { {{ 8.0, 2.54 }}, {{ 2.54, 8.0 }} },
    { {{ INF,  NaN }}, {{ NaN, INF }} } 
})

constexpr auto mse( auto a, auto b, auto c ) {
    auto se = []( auto a, auto b ) {
        return ( a - b ) * ( a - b );
    };
    return (se(a, 3) + se(b, 3) + se(c, 3)) / 3;
}

constexpr auto mseresidual( auto a, auto b, auto c ) {
    auto r = []( auto a, auto b ) {
        return 2 * ( a - b ) / 3;
    };
    return std::array{ r(a, 3), r(b, 3), r(c, 3) }; 
}

TEST_FUNCTION_GRADS_MATCH( MSE, mse, mseresidual, {
  {{ 3.0, 3.0, 3.0 }},
  {{ 9.0, 1.0, -3.0 }},
  {{ 3.0, 3.0, 0.0 }},
  {{ -10.0, 2.0, INF }},
  {{ NaN, 2.0, -1.0 }},
  {{ 0.0, 0.0, 0.0 }}
})