#pragma once

#include <slop/ops.hpp>
#include <slop/util.hpp>

#include <algorithm>
#include <cmath>
#include <concepts>
#include <cstdint>
#include <type_traits>
#include <vector>
#include <algorithm>
#include <ranges>

namespace slop {

using IndexType = uint64_t;

template < class Tape, class Floating >
struct Variable {
    Tape * tape_;
    Floating value_;
    IndexType id_;

    [[nodiscard]] constexpr Floating Grad() const { return tape_->GetGrad( *this ); }
};

template <class Floating>
struct Tape {
    using VarType = Variable<Tape<Floating>, Floating>;

    std::vector< Floating > grads_;
    std::vector< Floating > partials_;
    std::vector< IndexType > indices_;
    std::vector< OpType > opTypes_;

    [[nodiscard]] constexpr Variable<Tape<Floating>, Floating> Var( const Floating& value );
    [[nodiscard]] constexpr const Floating& GetGrad( const VarType& var) const noexcept;
    [[nodiscard]] constexpr const Floating& GetGrad( IndexType ix ) const noexcept;
    constexpr void Backward( const VarType& var );
};

template <class Floating>
[[nodiscard]] constexpr Variable<Tape<Floating>, Floating> Tape<Floating>::Var( const Floating& value ) {
    const auto id = grads_.size();
    grads_.emplace_back();
    return typename Tape<Floating>::VarType{ 
        .tape_ = this, 
        .value_ = value, 
        .id_ = id 
    };
}

template <class Floating>
[[nodiscard]] constexpr const Floating& Tape<Floating>::GetGrad( const VarType& var ) const noexcept {
    return GetGrad( var.id_ );
}

template <class Floating>
[[nodiscard]] constexpr const Floating& Tape<Floating>::GetGrad( IndexType ix ) const noexcept {
    return grads_[ ix ];
}

template <class Floating>
constexpr void Tape<Floating>::Backward( const typename Tape<Floating>::VarType& var ) {
    grads_[ var.id_ ] = static_cast<Floating>( 1 );
    auto indexPtr = indices_.rbegin();
    auto partialPtr = partials_.rbegin();
    for ( auto opPtr = opTypes_.rbegin(); opPtr != opTypes_.rend(); ++opPtr ) {
        switch ( *opPtr ) {
        case OpType::UNARY_OP: {
            const auto rgrad = grads_[ *indexPtr++ ];
            const auto partial = *partialPtr++; const auto ix = *indexPtr++;
            grads_[ ix ] += rgrad * partial;
            break;
        }
        case OpType::BINARY_OP: {
            const auto rgrad = grads_[ *indexPtr++ ];
            const auto ypartial = *partialPtr++; const auto yix = *indexPtr++;
            const auto xpartial = *partialPtr++; const auto xix = *indexPtr++;
            grads_[ yix ] += rgrad * ypartial;
            grads_[ xix ] += rgrad * xpartial;
            break;
        }
        case OpType::TERNARY_OP: {
            const auto rgrad = grads_[ *indexPtr++ ];
            const auto zpartial = *partialPtr++; const auto zix = *indexPtr++;
            const auto ypartial = *partialPtr++; const auto yix = *indexPtr++;
            const auto xpartial = *partialPtr++; const auto xix = *indexPtr++;
            grads_[ zix ] += rgrad * zpartial;
            grads_[ yix ] += rgrad * ypartial;
            grads_[ xix ] += rgrad * xpartial;
            break;
        }
        case OpType::NARY_OP: {
            const auto n = *indexPtr++;
            const auto& rgrad = grads_[ *indexPtr++ ];
            for ( auto i = 0u; i < n; ++i ) {
                const auto partial = *partialPtr++;
                const auto ix = *indexPtr++;
                grads_[ ix ] += rgrad * partial;
            }
            break;
        }
        default: {
            util::fail( "Unknown OpType {}", static_cast<uint64_t>(*opPtr) );
            break;
        }
        }
    }
}

#define DEFINE_BINOP( name_, arg1name_, arg2name_, valueexpr_, partial1_, partial2_ ) \
    template <class Floating> \
    [[nodiscard]] constexpr Variable<Tape<Floating>, Floating> name_( const Variable<Tape<Floating>, Floating>& p_ ## arg1name_, const Variable<Tape<Floating>, Floating>& p_ ## arg2name_ ) { \
        const auto arg1name_ = p_ ## arg1name_.value_; \
        const auto arg2name_ = p_ ## arg2name_.value_; \
        const auto tape = p_ ## arg1name_.tape_; \
        auto ret = tape->Var( ( valueexpr_) ); \
        tape->partials_.insert( tape->partials_.cend(), { partial1_, partial2_ } ); \
        tape->indices_.insert( tape->indices_.cend(), { p_ ## arg1name_.id_, p_ ## arg2name_.id_, ret.id_ } ); \
        tape->opTypes_.emplace_back( OpType::BINARY_OP ); \
        return ret; \
    }

DEFINE_BINOP( operator+, x, y, x + y, (static_cast<Floating>(1)), (static_cast<Floating>(1)) )
DEFINE_BINOP( operator-, x, y, x - y, (static_cast<Floating>(1)), (static_cast<Floating>(-1)) )
DEFINE_BINOP( operator*, x, y, x * y, y, x )
DEFINE_BINOP( operator/, x, y, x / y, 1 / y, - x / (y * y) )
DEFINE_BINOP( pow, x, y, std::pow(x, y), y * std::pow( x, y - 1 ), std::log( x ) * std::pow( x, y ) )

#undef DEFINE_BINOP

#define DEFINE_BINOP_CRHS( name_, arg1name_, arg2name_, valueexpr_, partial_ ) \
    template <class Floating, class T> \
    requires std::convertible_to< const T&, Floating > \
    [[nodiscard]] constexpr Variable<Tape<Floating>, Floating> name_( const Variable<Tape<Floating>, Floating>& p_ ## arg1name_, const T& p_ ## arg2name_ ) { \
        const auto arg1name_ = p_ ## arg1name_.value_; \
        const auto arg2name_ = static_cast<Floating>( p_ ## arg2name_ ); \
        const auto tape = p_ ## arg1name_.tape_; \
        auto ret = tape->Var( ( valueexpr_) ); \
        tape->partials_.emplace_back( (partial_) ); \
        tape->indices_.insert( tape->indices_.cend(), { p_ ## arg1name_.id_, ret.id_ } ); \
        tape->opTypes_.emplace_back( OpType::UNARY_OP ); \
        return ret; \
    }

DEFINE_BINOP_CRHS( operator+, x, y, x + y, (static_cast<Floating>(1)) )
DEFINE_BINOP_CRHS( operator-, x, y, x - y, (static_cast<Floating>(1)) )
DEFINE_BINOP_CRHS( operator*, x, y, x * y, y )
DEFINE_BINOP_CRHS( operator/, x, y, x / y, 1 / y )
DEFINE_BINOP_CRHS( pow, x, y, std::pow(x, y), y * std::pow( x, y - 1 ) )

#undef DEFINE_BINOP_CRHS


#define DEFINE_BINOP_CLHS( name_, arg1name_, arg2name_, valueexpr_, partial_ ) \
    template <class Floating, class T> \
    requires std::convertible_to< const T&, Floating > \
    [[nodiscard]] constexpr Variable<Tape<Floating>, Floating> name_( const T& p_ ## arg1name_, const Variable<Tape<Floating>, Floating>& p_ ## arg2name_ ) { \
        const auto arg2name_ = p_ ## arg2name_.value_; \
        const auto arg1name_ = static_cast<Floating>( p_ ## arg1name_ ); \
        const auto tape = p_ ## arg2name_.tape_; \
        auto ret = tape->Var( ( valueexpr_) ); \
        tape->partials_.emplace_back( (partial_) ); \
        tape->indices_.insert( tape->indices_.cend(), { p_ ## arg2name_.id_, ret.id_ } ); \
        tape->opTypes_.emplace_back( OpType::UNARY_OP ); \
        return ret; \
    }

DEFINE_BINOP_CLHS( operator+, x, y, x + y, (static_cast<Floating>(1)) )
DEFINE_BINOP_CLHS( operator-, x, y, x - y, (static_cast<Floating>(-1)) )
DEFINE_BINOP_CLHS( operator*, x, y, x * y, x )
DEFINE_BINOP_CLHS( operator/, x, y, x / y, - x / (y * y) )
DEFINE_BINOP_CLHS( pow, x, y, std::pow(x, y), std::log( x ) * std::pow( x, y ) )

#undef DEFINE_BINOP_CLHS

#define DEFINE_UNOP( name_, argname_, valueexpr_, partial_ ) \
    template <class Floating> \
    [[nodiscard]] constexpr Variable<Tape<Floating>, Floating> name_( const Variable<Tape<Floating>, Floating>& p_ ## argname_) { \
        const auto argname_ = p_ ## argname_.value_; \
        const auto tape = p_ ## argname_.tape_; \
        auto ret = tape->Var( ( valueexpr_) ); \
        tape->partials_.emplace_back( (partial_) ); \
        tape->indices_.insert( tape->indices_.cend(), { p_ ## argname_.id_, ret.id_ } ); \
        tape->opTypes_.emplace_back( OpType::UNARY_OP ); \
        return ret; \
    }

DEFINE_UNOP( operator-, x, -x, (static_cast<Floating>(-1)) )
DEFINE_UNOP( exp, x, std::exp(x), x)
DEFINE_UNOP( log, x, std::log(x), 1 / x)
DEFINE_UNOP( sqrt, x, std::sqrt(x), 1 / (2 * std::sqrt(x)) )

#undef DEFINE_UNOP

}
