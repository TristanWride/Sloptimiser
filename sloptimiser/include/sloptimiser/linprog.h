#include <algorithm>
#include <vector>
#include <span>
#include <ranges>
#include <numeric>

namespace sloptimiser::linprog {

template <typename F>
class Tableaux {
public:
    using ValueType = F;

    [[nodiscard]] explicit constexpr Tableaux(std::size_t numVars, std::size_t numConstraints)
        : tab_((numVars + 2) * (numConstraints + 1), static_cast<F>(0)) {}

    virtual ~Tableaux() noexcept = default;

private:
    std::vector<F> tab_ ; // Columns are variables, rows are constraints + objective row
};

template <typename Tab>
class LinearProgram {
public:
    std::size_t numVars, numConstraints;
    using VariableID = std::size_t;
    using ValueType = typename Tab::ValueType;

    struct LinearTerm { 
        LinearProgram<Tab>* lp;
        VariableID id;
        ValueType coeff;

        LinearTerm& operator*=(ValueType v) {
            coeff *= v;
            return *this;
        }

        LinearTerm& operator/=(ValueType v) {
            coeff /= v;
            return *this;
        }
    };

    struct VarRef {
        LinearProgram<Tab>* lp;
        VariableID id;

        friend LinearTerm operator*(const ValueType& v, const VarRef& ref) {
            return LinearTerm{ref.lp, ref.id, v};
        }

        operator LinearTerm() const {
            return LinearTerm{lp, id, static_cast<ValueType>(1)};
        }
    };


    class LinearExpression {
        LinearProgram<Tab>* lp;
        std::vector<ValueType> coeffs;

        LinearExpression(LinearProgram<Tab>& lp) 
            : lp{lp}, coeffs(lp.numVars, static_cast<Tab::ValueType>(0))
        {}

        LinearExpression& operator+=(const LinearTerm& lt) {
            coeffs.at(lt.id) += lt.coeff;
            return *this;
        }

        LinearExpression& operator-=(const LinearTerm& lt) {
            coeffs.at(lt.id) -= lt.coeff;
            return *this;
        }

        LinearExpression& operator*=(ValueType v) {
            std::transform(coeffs, coeffs.begin(), [&] (auto c) { return c * v; });
            return *this;
        }

        LinearExpression& operator/=(ValueType v) {
            std::transform(coeffs, coeffs.begin(), [&] (auto c) { return c / v; });
            return *this;
        }
    };

    [[nodiscard]] explicit constexpr LinearProgram() : numVars{0}, numConstraints{0} {}

    [[nodiscard]] VarRef NewVar() {
        const auto outVar = static_cast<VariableID>(numVars);
        ++numVars;
        return VarRef{this, outVar};
    }

    [[nodiscard]] std::vector<VarRef> NewVarVec(std::size_t size) {
        auto outVar = std::vector<VarRef>(size);

        for (auto i = 0u; i < size; ++i) {
            outVar[i] = VarRef{this, numVars++};
        }

        return outVar;
    }

    virtual ~LinearProgram() noexcept = default;
};

}
