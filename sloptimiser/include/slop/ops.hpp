#pragma once

#include <cstdint>

namespace slop {

enum struct OpType : uint8_t {
    UNARY_OP,
    BINARY_OP,
    TERNARY_OP,
    NARY_OP
};

}