#include <slop/slop.hpp>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <array>
#include <vector>
#include <limits>
#include <tuple>
#include <format>
#include <cassert>

static constexpr double PRECISE_EPS = std::numeric_limits<double>::epsilon();
static constexpr double NaN = std::numeric_limits<double>::quiet_NaN();
static constexpr double INF = std::numeric_limits<double>::infinity();

template < class T,  uint64_t Rank >
struct RankVector {
    using Type = std::vector< typename RankVector<T, Rank - 1>::Type >;
};

template < class T >
struct RankVector< T, 0 > {
    using Type = T;
};

template < class T, uint64_t Rank >
using RankVectorT = typename RankVector<T, Rank>::Type;

template < class T, uint64_t Rank >
consteval std::array< uint64_t, Rank > GetDims( RankVectorT< T, Rank > vecs ) {
    if constexpr ( Rank == 0 ) {
        return std::array<uint64_t, Rank>{};
    } else {
        auto sub = GetDims<T, Rank - 1>(vecs[0]);
        for ( auto sub_ : vecs ) 
            assert(sub == (GetDims<T, Rank - 1>( sub_ )));
        return std::apply([&]( const auto&... rs ) { return std::array{ vecs.size(), rs... }; }, sub);
    }
}

TEST( TestHelper, GetDimsHelper ) {
    static constexpr auto dims1 = GetDims<int, 3>({
        { {{ 0, 1 }}, {{ 1, 5 }} },
        { {{ 0, 1 }}, {{ 1, 5 }} },
        { {{ 0, 1 }}, {{ 1, 5 }} },
        { {{ 0, 1 }}, {{ 1, 5 }} },
        { {{ 0, 1 }}, {{ 1, 5 }} },
        { {{ 0, 1 }}, {{ 1, 5 }} }
    });

    EXPECT_THAT( dims1, ::testing::Pointwise( ::testing::Eq(), std::array{ 6, 2, 2 } ));

    static constexpr auto dims2 = GetDims<double, 2>({
        { 3.0, 3.0, 3.0 },
        { 9.0, 1.0, -3.0 },
        { 3.0, 3.0, 0.0 },
        { -10.0, 2.0, INF },
        { NaN, 2.0, -1.0 },
        { 0.0, NaN, INF },
        { 0.0, 0.0, 0.0 }
    });

    EXPECT_THAT( dims2, ::testing::Pointwise( ::testing::Eq(), std::array{ 7, 3 } ));
}


#define TEST_FUNCTION_GRADS( name_, func_, ... ) \
    TEST ( BasicFunction, Test ## name_ ) { \
        static constexpr auto dims = GetDims< double, 3 >(__VA_ARGS__); \
        static constexpr auto arity = std::get<2>(dims); \
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
        static constexpr auto dims = GetDims<double, 2>(__VA_ARGS__); \
        static constexpr auto arity = std::get<1>(dims); \
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
    { { 3.0,  6.0 }, { 1.0, 1.0 } },
    { { 9.0, -2.0 }, { 1.0, 1.0 } },
    { { 8.0, 2.54 }, { 1.0, 1.0 } },
    { { INF,  NaN }, { 1.0, 1.0 } } 
})

TEST_FUNCTION_GRADS( Multiply, []( auto a, auto b ) { return a * b; }, {
    { { 3.0,  6.0 }, { 6.0, 3.0  } },
    { { 9.0, -2.0 }, { -2.0, 9.0 } },
    { { 8.0, 2.54 }, { 2.54, 8.0 } },
    { { INF,  NaN }, { NaN, INF  } } 
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
  { 3.0, 3.0, 3.0 },
  { 9.0, 1.0, -3.0 },
  { 3.0, 3.0, 0.0 },
  { -10.0, 2.0, INF },
  { NaN, 2.0, -1.0 },
  { 0.0, NaN, INF },
  { 0.0, 0.0, 0.0 }
})