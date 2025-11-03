#include "sloptimiser/linprog.h"

#include <gtest/gtest.h>

#include <set>
#include <format>
#include <ranges>

using sloptimiser::linprog::Tableaux;
using sloptimiser::linprog::LinearProgram;


TEST (linprog, LinearProgramConstruction) {
    auto lp = LinearProgram<Tableaux<int>>();
    using VariableID = LinearProgram<Tableaux<int>>::VariableID;

    auto seenVars = std::set<VariableID>{};

    EXPECT_EQ(lp.numVars, 0u);

    auto x = lp.NewVar();
    seenVars.emplace(x.id);

    EXPECT_EQ(lp.numVars, 1u);
    EXPECT_EQ(x.id, 0u);

    const auto VecSize = 13u;
    auto zs = lp.NewVarVec(VecSize);

    for (const auto [i, z] : zs | std::views::enumerate) {
        SCOPED_TRACE(std::format("zs[{}] = {}", i, z.id));
        auto [iter, success] = seenVars.emplace(z.id);
        EXPECT_TRUE(success);
    }

    EXPECT_EQ(lp.numVars, (1 + VecSize));
    
    for (auto x_ : seenVars) {
        EXPECT_GE(x_, 0u);
        EXPECT_LT(x_, lp.numVars);
    }
}