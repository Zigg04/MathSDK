#include "symbolic_integral.h"

#include <symengine/add.h>
#include <symengine/constants.h>
#include <symengine/derivative.h>
#include <symengine/functions.h>
#include <symengine/mul.h>
#include <symengine/pow.h>
#include <symengine/rational.h>
#include <symengine/visitor.h>

namespace mathsdk
{

namespace
{

using SymEngine::Basic;
using SymEngine::RCP;
using SymEngine::Symbol;

bool depends_on(const RCP<const Basic> &expr, const RCP<const Symbol> &x)
{
    return SymEngine::has_symbol(*expr, *x);
}

RCP<const Basic> integrate(
    const RCP<const Basic> &expr,
    const RCP<const Symbol> &x
);

RCP<const Basic> power_rule(const RCP<const Basic> &base, const RCP<const Basic> &exponent)
{
    const auto new_exponent = SymEngine::add(exponent, SymEngine::one);
    return SymEngine::div(SymEngine::pow(base, new_exponent), new_exponent);
}

RCP<const Basic> linear_coefficient_inverse(
    const RCP<const Basic> &arg,
    const RCP<const Symbol> &x
)
{
    if (SymEngine::eq(*arg, *x)) {
        return SymEngine::one;
    }
    const auto derivative = SymEngine::diff(arg, x);
    if (depends_on(derivative, x)) {
        return SymEngine::RCP<const Basic>();
    }
    if (SymEngine::eq(*derivative, *SymEngine::zero)) {
        return SymEngine::RCP<const Basic>();
    }
    return SymEngine::div(SymEngine::one, derivative);
}


RCP<const Basic> linear_substitution(
    const RCP<const Basic> &arg,
    const RCP<const Symbol> &x,
    const RCP<const Basic> &table_antiderivative_of_arg
)
{
    const auto inverse_slope = linear_coefficient_inverse(arg, x);
    if (inverse_slope.is_null()) {
        return SymEngine::RCP<const Basic>();
    }
    return SymEngine::mul(inverse_slope, table_antiderivative_of_arg);
}

RCP<const Basic> integrate_one_arg_function(
    const RCP<const Basic> &expr,
    const RCP<const Symbol> &x
)
{
    if (SymEngine::is_a<SymEngine::Sin>(*expr)) {
        const auto arg = SymEngine::down_cast<const SymEngine::Sin &>(*expr).get_arg();
        return linear_substitution(arg, x, SymEngine::neg(SymEngine::cos(arg)));
    }
    if (SymEngine::is_a<SymEngine::Cos>(*expr)) {
        const auto arg = SymEngine::down_cast<const SymEngine::Cos &>(*expr).get_arg();
        return linear_substitution(arg, x, SymEngine::sin(arg));
    }
    return SymEngine::RCP<const Basic>();
}

RCP<const Basic> integrate_pow(
    const RCP<const Basic> &base,
    const RCP<const Basic> &exponent,
    const RCP<const Symbol> &x
)
{
    if (!depends_on(base, x) && depends_on(exponent, x)) {
        const auto inverse_slope = linear_coefficient_inverse(exponent, x);
        if (inverse_slope.is_null()) {
            return SymEngine::RCP<const Basic>();
        }
        const auto original = SymEngine::pow(base, exponent);
        if (SymEngine::eq(*base, *SymEngine::E)) {
            return SymEngine::mul(inverse_slope, original);
        }
        return SymEngine::mul(inverse_slope, SymEngine::div(original, SymEngine::log(base)));
    }

    if (depends_on(base, x) && !depends_on(exponent, x)) {
        if (SymEngine::eq(*base, *x)) {
            if (SymEngine::eq(*exponent, *SymEngine::minus_one)) {
                return SymEngine::log(x);
            }
            return power_rule(base, exponent);
        }
        const auto inverse_slope = linear_coefficient_inverse(base, x);
        if (inverse_slope.is_null() || SymEngine::eq(*exponent, *SymEngine::minus_one)) {
            return SymEngine::RCP<const Basic>();
        }
        return SymEngine::mul(inverse_slope, power_rule(base, exponent));
    }

    return SymEngine::RCP<const Basic>();
}

RCP<const Basic> integrate(
    const RCP<const Basic> &expr,
    const RCP<const Symbol> &x
)
{
    if (!depends_on(expr, x)) {
        return SymEngine::mul(expr, x);
    }
    if (SymEngine::eq(*expr, *x)) {
        return SymEngine::div(SymEngine::pow(x, SymEngine::integer(2)), SymEngine::integer(2));
    }

    if (SymEngine::is_a<SymEngine::Add>(*expr)) {
        RCP<const Basic> sum = SymEngine::zero;
        for (const auto &term : expr->get_args()) {
            const auto term_integral = integrate(term, x);
            if (term_integral.is_null()) {
                return SymEngine::RCP<const Basic>();
            }
            sum = SymEngine::add(sum, term_integral);
        }
        return sum;
    }

    if (SymEngine::is_a<SymEngine::Mul>(*expr)) {
        RCP<const Basic> constant_part = SymEngine::one;
        RCP<const Basic> variable_part = SymEngine::one;
        for (const auto &factor : expr->get_args()) {
            if (depends_on(factor, x)) {
                variable_part = SymEngine::mul(variable_part, factor);
            } else {
                constant_part = SymEngine::mul(constant_part, factor);
            }
        }
        if (SymEngine::eq(*variable_part, *SymEngine::one)) {
            return SymEngine::mul(expr, x);
        }
        if (SymEngine::eq(*constant_part, *SymEngine::one)) {
            return SymEngine::RCP<const Basic>();
        }
        const auto variable_integral = integrate(variable_part, x);
        if (variable_integral.is_null()) {
            return SymEngine::RCP<const Basic>();
        }
        return SymEngine::mul(constant_part, variable_integral);
    }

    if (SymEngine::is_a<SymEngine::Pow>(*expr)) {
        const auto &pow_expr = SymEngine::down_cast<const SymEngine::Pow &>(*expr);
        return integrate_pow(pow_expr.get_base(), pow_expr.get_exp(), x);
    }

    return integrate_one_arg_function(expr, x);
}

} // namespace

SymEngine::RCP<const SymEngine::Basic> try_integrate(
    const SymEngine::RCP<const SymEngine::Basic> &expr,
    const SymEngine::RCP<const SymEngine::Symbol> &variable
)
{
    const auto candidate = integrate(expr, variable);
    if (candidate.is_null()) {
        return candidate;
    }

    const auto check = SymEngine::diff(candidate, variable);
    const auto difference = SymEngine::expand(SymEngine::sub(check, expr));
    if (!SymEngine::eq(*difference, *SymEngine::zero)) {
        return SymEngine::RCP<const SymEngine::Basic>();
    }
    return candidate;
}

} // namespace mathsdk
