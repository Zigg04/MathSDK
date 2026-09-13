#include "mathsdk/math_sdk.h"

#include <cctype>
#include <cstdlib>
#include <cstring>
#include <exception>
#include <stdexcept>
#include <string>
#include <vector>


#ifndef BOOST_ALL_NO_LIB
#define BOOST_ALL_NO_LIB
#endif
#include <boost/json/src.hpp>

#include <symengine/add.h>
#include <symengine/constants.h>
#include <symengine/derivative.h>
#include <symengine/eval_double.h>
#include <symengine/matrix.h>
#include <symengine/mul.h>
#include <symengine/parser.h>
#include <symengine/series_generic.h>
#include <symengine/sets.h>
#include <symengine/solve.h>
#include <symengine/subs.h>
#include <symengine/symbol.h>
#include <symengine/symengine_exception.h>

#include "arithmetic_steps.h"
#include "symbolic_integral.h"

#ifndef MATHSDK_VERSION
#define MATHSDK_VERSION "0.1.0"
#endif

namespace
{

char *copy_string(const char *value) noexcept
{
    const auto size = std::strlen(value);
    auto *copy = static_cast<char *>(std::malloc(size + 1));
    if (copy == nullptr) {
        return nullptr;
    }

    std::memcpy(copy, value, size + 1);
    return copy;
}

bool is_blank(const char *value) noexcept
{
    if (value == nullptr) {
        return true;
    }

    for (const auto *cursor = value; *cursor != '\0'; ++cursor) {
        if (!std::isspace(static_cast<unsigned char>(*cursor))) {
            return false;
        }
    }
    return true;
}

MathSdkStatus write_error(
    MathSdkStatus status,
    const char *message,
    char **out_error
) noexcept
{
    *out_error = copy_string(message);
    return *out_error == nullptr ? MATHSDK_STATUS_INTERNAL_ERROR : status;
}

MathSdkStatus map_symengine_status(symengine_exceptions_t status) noexcept
{
    switch (status) {
    case SYMENGINE_PARSE_ERROR:
        return MATHSDK_STATUS_PARSE_ERROR;
    case SYMENGINE_DIV_BY_ZERO:
        return MATHSDK_STATUS_DIVISION_BY_ZERO;
    case SYMENGINE_DOMAIN_ERROR:
        return MATHSDK_STATUS_DOMAIN_ERROR;
    case SYMENGINE_NOT_IMPLEMENTED:
        return MATHSDK_STATUS_NOT_IMPLEMENTED;
    default:
        return MATHSDK_STATUS_INTERNAL_ERROR;
    }
}

MathSdkStatus symengine_canonicalize(
    const char *expression,
    std::string &out_canonical,
    char **out_error
) noexcept
{
    try {
        const auto parsed = SymEngine::parse(expression);
        if (SymEngine::eq(*parsed, *SymEngine::ComplexInf)) {
            return write_error(
                MATHSDK_STATUS_DIVISION_BY_ZERO,
                "Division by zero",
                out_error
            );
        }
        if (SymEngine::eq(*parsed, *SymEngine::Nan)) {
            return write_error(
                MATHSDK_STATUS_DOMAIN_ERROR,
                "Expression is undefined",
                out_error
            );
        }
        out_canonical = parsed->__str__();
        return MATHSDK_STATUS_OK;
    } catch (SymEngine::SymEngineException &error) {
        return write_error(
            map_symengine_status(error.error_code()),
            error.what(),
            out_error
        );
    } catch (const std::exception &error) {
        return write_error(MATHSDK_STATUS_INTERNAL_ERROR, error.what(), out_error);
    } catch (...) {
        return write_error(
            MATHSDK_STATUS_INTERNAL_ERROR,
            "Unknown native error",
            out_error
        );
    }
}

std::vector<std::string> split(const std::string &text, char delimiter)
{
    std::vector<std::string> parts;
    std::size_t start = 0;
    while (start <= text.size()) {
        const auto pos = text.find(delimiter, start);
        const auto end = pos == std::string::npos ? text.size() : pos;
        parts.push_back(text.substr(start, end - start));
        if (pos == std::string::npos) break;
        start = pos + 1;
    }
    return parts;
}

SymEngine::RCP<const SymEngine::Basic> parse_zero_form(const std::string &equation)
{
    const auto eq_pos = equation.find('=');
    const auto lhs_str = equation.substr(0, eq_pos);
    const auto rhs_str = eq_pos == std::string::npos
        ? std::string("0")
        : equation.substr(eq_pos + 1);
    return SymEngine::sub(SymEngine::parse(lhs_str), SymEngine::parse(rhs_str));
}

unsigned parse_dimension(const std::string &text)
{
    try {
        return static_cast<unsigned>(std::stoul(text));
    } catch (const std::invalid_argument &) {
        throw std::invalid_argument("rows and cols must be numbers, got \"" + text + "\"");
    } catch (const std::out_of_range &) {
        throw std::invalid_argument("rows/cols value out of range: \"" + text + "\"");
    }
}

SymEngine::DenseMatrix parse_matrix(const std::string &text)
{
    const auto parts = split(text, ';');
    if (parts.size() != 3) {
        throw std::invalid_argument(
            "matrix must be \"rows;cols;v0,v1,...,vN\""
        );
    }
    const auto rows = parse_dimension(parts[0]);
    const auto cols = parse_dimension(parts[1]);
    if (rows == 0 || cols == 0) {
        throw std::invalid_argument("rows and cols must be positive");
    }

    SymEngine::vec_basic values;
    for (const auto &value : split(parts[2], ',')) {
        values.push_back(SymEngine::parse(value));
    }
    if (values.size() != static_cast<std::size_t>(rows) * cols) {
        throw std::invalid_argument("value count does not match rows*cols");
    }

    return SymEngine::DenseMatrix(rows, cols, values);
}

std::string serialize_matrix(const SymEngine::DenseMatrix &matrix)
{
    std::string result = std::to_string(matrix.nrows()) + ";"
        + std::to_string(matrix.ncols()) + ";";
    for (unsigned i = 0; i < matrix.nrows(); ++i) {
        for (unsigned j = 0; j < matrix.ncols(); ++j) {
            if (i != 0 || j != 0) {
                result += ",";
            }
            result += matrix.get(i, j)->__str__();
        }
    }
    return result;
}

} // namespace

const char *math_sdk_version(void) noexcept
{
    return MATHSDK_VERSION;
}

MathSdkStatus math_sdk_evaluate(
    const char *expression,
    char **out_result,
    char **out_error
) noexcept
{
    if (out_result == nullptr || out_error == nullptr) {
        return MATHSDK_STATUS_INVALID_ARGUMENT;
    }

    *out_result = nullptr;
    *out_error = nullptr;

    if (is_blank(expression)) {
        return write_error(
            MATHSDK_STATUS_INVALID_ARGUMENT,
            "Expression cannot be empty",
            out_error
        );
    }

    std::string canonical;
    const auto status = symengine_canonicalize(expression, canonical, out_error);
    if (status != MATHSDK_STATUS_OK) {
        return status;
    }

    *out_result = copy_string(canonical.c_str());
    if (*out_result == nullptr) {
        return write_error(
            MATHSDK_STATUS_INTERNAL_ERROR,
            "Unable to allocate result",
            out_error
        );
    }
    return MATHSDK_STATUS_OK;
}

MathSdkStatus math_sdk_evaluate_steps(
    const char *expression,
    char **out_result,
    char **out_steps,
    char **out_error
) noexcept
{
    if (out_result == nullptr || out_steps == nullptr || out_error == nullptr) {
        return MATHSDK_STATUS_INVALID_ARGUMENT;
    }

    *out_result = nullptr;
    *out_steps = nullptr;
    *out_error = nullptr;

    if (is_blank(expression)) {
        return write_error(
            MATHSDK_STATUS_INVALID_ARGUMENT,
            "Expression cannot be empty",
            out_error
        );
    }

    std::string canonical;
    const auto canonical_status = symengine_canonicalize(expression, canonical, out_error);
    if (canonical_status != MATHSDK_STATUS_OK) {
        return canonical_status;
    }

    std::string steps_json;
    std::string steps_final_result;
    try {
        steps_json = mathsdk::arithmetic_steps_to_json(expression, steps_final_result);
    } catch (const mathsdk::ArithmeticStepsError &error) {
        return write_error(MATHSDK_STATUS_NOT_IMPLEMENTED, error.what(), out_error);
    } catch (const std::exception &error) {
        return write_error(MATHSDK_STATUS_INTERNAL_ERROR, error.what(), out_error);
    } catch (...) {
        return write_error(
            MATHSDK_STATUS_INTERNAL_ERROR,
            "Unknown native error",
            out_error
        );
    }

    if (canonical != steps_final_result) {
        return write_error(
            MATHSDK_STATUS_INTERNAL_ERROR,
            "Step trace result disagrees with the canonical evaluation",
            out_error
        );
    }

    *out_result = copy_string(canonical.c_str());
    *out_steps = copy_string(steps_json.c_str());
    if (*out_result == nullptr || *out_steps == nullptr) {
        std::free(*out_result);
        std::free(*out_steps);
        *out_result = nullptr;
        *out_steps = nullptr;
        return write_error(
            MATHSDK_STATUS_INTERNAL_ERROR,
            "Unable to allocate result",
            out_error
        );
    }
    return MATHSDK_STATUS_OK;
}

MathSdkStatus math_sdk_substitute(
    const char *expression,
    const char *variable,
    const char *value,
    char **out_result,
    char **out_error
) noexcept
{
    if (out_result == nullptr || out_error == nullptr) {
        return MATHSDK_STATUS_INVALID_ARGUMENT;
    }

    *out_result = nullptr;
    *out_error = nullptr;

    if (is_blank(expression) || is_blank(variable) || is_blank(value)) {
        return write_error(
            MATHSDK_STATUS_INVALID_ARGUMENT,
            "expression, variable, and value are all required",
            out_error
        );
    }

    try {
        const auto parsed_expr = SymEngine::parse(expression);
        const auto parsed_var = SymEngine::parse(variable);
        if (!SymEngine::is_a<SymEngine::Symbol>(*parsed_var)) {
            return write_error(
                MATHSDK_STATUS_INVALID_ARGUMENT,
                "variable must be a single symbol, e.g. \"x\"",
                out_error
            );
        }
        const auto parsed_value = SymEngine::parse(value);

        const SymEngine::map_basic_basic replacement{{parsed_var, parsed_value}};
        const auto substituted = SymEngine::subs(parsed_expr, replacement);

        if (SymEngine::eq(*substituted, *SymEngine::ComplexInf)) {
            return write_error(MATHSDK_STATUS_DIVISION_BY_ZERO, "Division by zero", out_error);
        }
        if (SymEngine::eq(*substituted, *SymEngine::Nan)) {
            return write_error(MATHSDK_STATUS_DOMAIN_ERROR, "Expression is undefined", out_error);
        }

        const auto result = substituted->__str__();
        *out_result = copy_string(result.c_str());
        if (*out_result == nullptr) {
            return write_error(MATHSDK_STATUS_INTERNAL_ERROR, "Unable to allocate result", out_error);
        }
        return MATHSDK_STATUS_OK;
    } catch (SymEngine::SymEngineException &error) {
        return write_error(map_symengine_status(error.error_code()), error.what(), out_error);
    } catch (const std::exception &error) {
        return write_error(MATHSDK_STATUS_NOT_IMPLEMENTED, error.what(), out_error);
    } catch (...) {
        return write_error(MATHSDK_STATUS_INTERNAL_ERROR, "Unknown native error", out_error);
    }
}

MathSdkStatus math_sdk_evaluate_numeric(
    const char *expression,
    double *out_result,
    char **out_error
) noexcept
{
    if (out_result == nullptr || out_error == nullptr) {
        return MATHSDK_STATUS_INVALID_ARGUMENT;
    }

    *out_result = 0.0;
    *out_error = nullptr;

    if (is_blank(expression)) {
        return write_error(
            MATHSDK_STATUS_INVALID_ARGUMENT,
            "Expression cannot be empty",
            out_error
        );
    }

    SymEngine::RCP<const SymEngine::Basic> parsed;
    try {
        parsed = SymEngine::parse(expression);
    } catch (SymEngine::SymEngineException &error) {
        return write_error(map_symengine_status(error.error_code()), error.what(), out_error);
    } catch (const std::exception &error) {
        return write_error(MATHSDK_STATUS_INTERNAL_ERROR, error.what(), out_error);
    } catch (...) {
        return write_error(MATHSDK_STATUS_INTERNAL_ERROR, "Unknown native error", out_error);
    }

    if (SymEngine::eq(*parsed, *SymEngine::ComplexInf)) {
        return write_error(MATHSDK_STATUS_DIVISION_BY_ZERO, "Division by zero", out_error);
    }
    if (SymEngine::eq(*parsed, *SymEngine::Nan)) {
        return write_error(MATHSDK_STATUS_DOMAIN_ERROR, "Expression is undefined", out_error);
    }

    try {
        *out_result = SymEngine::eval_double(*parsed);
        return MATHSDK_STATUS_OK;
    } catch (const std::exception &error) {
        return write_error(MATHSDK_STATUS_NOT_IMPLEMENTED, error.what(), out_error);
    } catch (...) {
        return write_error(MATHSDK_STATUS_INTERNAL_ERROR, "Unknown native error", out_error);
    }
}

MathSdkStatus math_sdk_derivative(
    const char *expression,
    const char *variable,
    int order,
    char **out_result,
    char **out_error
) noexcept
{
    if (out_result == nullptr || out_error == nullptr) {
        return MATHSDK_STATUS_INVALID_ARGUMENT;
    }

    *out_result = nullptr;
    *out_error = nullptr;

    if (is_blank(expression) || is_blank(variable)) {
        return write_error(
            MATHSDK_STATUS_INVALID_ARGUMENT,
            "expression and variable are required",
            out_error
        );
    }
    if (order < 1 || order > 10) {
        return write_error(
            MATHSDK_STATUS_INVALID_ARGUMENT,
            "order must be between 1 and 10",
            out_error
        );
    }

    try {
        const auto parsed_expr = SymEngine::parse(expression);
        const auto parsed_var = SymEngine::parse(variable);
        if (!SymEngine::is_a<SymEngine::Symbol>(*parsed_var)) {
            return write_error(
                MATHSDK_STATUS_INVALID_ARGUMENT,
                "variable must be a single symbol, e.g. \"x\"",
                out_error
            );
        }
        const auto symbol = SymEngine::rcp_static_cast<const SymEngine::Symbol>(parsed_var);

        auto derived = parsed_expr;
        for (int i = 0; i < order; ++i) {
            derived = SymEngine::diff(derived, symbol);
        }

        const auto result = derived->__str__();
        *out_result = copy_string(result.c_str());
        if (*out_result == nullptr) {
            return write_error(MATHSDK_STATUS_INTERNAL_ERROR, "Unable to allocate result", out_error);
        }
        return MATHSDK_STATUS_OK;
    } catch (SymEngine::SymEngineException &error) {
        return write_error(map_symengine_status(error.error_code()), error.what(), out_error);
    } catch (const std::exception &error) {
        return write_error(MATHSDK_STATUS_NOT_IMPLEMENTED, error.what(), out_error);
    } catch (...) {
        return write_error(MATHSDK_STATUS_INTERNAL_ERROR, "Unknown native error", out_error);
    }
}

MathSdkStatus math_sdk_solve(
    const char *equation,
    const char *variable,
    char **out_result,
    char **out_error
) noexcept
{
    if (out_result == nullptr || out_error == nullptr) {
        return MATHSDK_STATUS_INVALID_ARGUMENT;
    }

    *out_result = nullptr;
    *out_error = nullptr;

    if (is_blank(equation) || is_blank(variable)) {
        return write_error(
            MATHSDK_STATUS_INVALID_ARGUMENT,
            "equation and variable are required",
            out_error
        );
    }

    try {
        const auto parsed_var = SymEngine::parse(variable);
        if (!SymEngine::is_a<SymEngine::Symbol>(*parsed_var)) {
            return write_error(
                MATHSDK_STATUS_INVALID_ARGUMENT,
                "variable must be a single symbol, e.g. \"x\"",
                out_error
            );
        }
        const auto symbol = SymEngine::rcp_static_cast<const SymEngine::Symbol>(parsed_var);

        const auto zero_form = parse_zero_form(equation);
        const auto solution_set = SymEngine::solve(zero_form, symbol);

        if (SymEngine::is_a<SymEngine::EmptySet>(*solution_set)) {
            *out_result = copy_string("[]");
            return *out_result == nullptr
                ? write_error(MATHSDK_STATUS_INTERNAL_ERROR, "Unable to allocate result", out_error)
                : MATHSDK_STATUS_OK;
        }
        if (!SymEngine::is_a<SymEngine::FiniteSet>(*solution_set)) {
            return write_error(
                MATHSDK_STATUS_NOT_IMPLEMENTED,
                "Solution set is infinite or symbolic; only finite root sets are supported",
                out_error
            );
        }

        boost::json::array roots;
        for (const auto &root : solution_set->get_args()) {
            roots.push_back(boost::json::string(root->__str__()));
        }
        const auto result = boost::json::serialize(roots);
        *out_result = copy_string(result.c_str());
        if (*out_result == nullptr) {
            return write_error(MATHSDK_STATUS_INTERNAL_ERROR, "Unable to allocate result", out_error);
        }
        return MATHSDK_STATUS_OK;
    } catch (SymEngine::SymEngineException &error) {
        return write_error(map_symengine_status(error.error_code()), error.what(), out_error);
    } catch (const std::exception &error) {
        return write_error(MATHSDK_STATUS_NOT_IMPLEMENTED, error.what(), out_error);
    } catch (...) {
        return write_error(MATHSDK_STATUS_INTERNAL_ERROR, "Unknown native error", out_error);
    }
}

MathSdkStatus math_sdk_solve_linear_system(
    const char *equations,
    const char *variables,
    char **out_result,
    char **out_error
) noexcept
{
    if (out_result == nullptr || out_error == nullptr) {
        return MATHSDK_STATUS_INVALID_ARGUMENT;
    }

    *out_result = nullptr;
    *out_error = nullptr;

    if (is_blank(equations) || is_blank(variables)) {
        return write_error(
            MATHSDK_STATUS_INVALID_ARGUMENT,
            "equations and variables are required",
            out_error
        );
    }

    try {
        SymEngine::vec_sym symbols;
        for (const auto &name : split(variables, ',')) {
            const auto parsed_var = SymEngine::parse(name);
            if (!SymEngine::is_a<SymEngine::Symbol>(*parsed_var)) {
                return write_error(
                    MATHSDK_STATUS_INVALID_ARGUMENT,
                    "each variable must be a single symbol, e.g. \"x,y\"",
                    out_error
                );
            }
            symbols.push_back(SymEngine::rcp_static_cast<const SymEngine::Symbol>(parsed_var));
        }

        SymEngine::vec_basic system;
        for (const auto &equation : split(equations, ';')) {
            system.push_back(parse_zero_form(equation));
        }

        if (system.size() != symbols.size()) {
            return write_error(
                MATHSDK_STATUS_INVALID_ARGUMENT,
                "the number of equations must match the number of variables",
                out_error
            );
        }

        const auto matrix_form = SymEngine::linear_eqns_to_matrix(system, symbols);
        const auto determinant = matrix_form.first.det();
        if (SymEngine::eq(*determinant, *SymEngine::zero)) {
            return write_error(
                MATHSDK_STATUS_NOT_IMPLEMENTED,
                "system has no unique solution (inconsistent or underdetermined)",
                out_error
            );
        }

        const auto solution = SymEngine::linsolve(system, symbols);
        if (solution.size() != symbols.size()) {
            return write_error(MATHSDK_STATUS_INTERNAL_ERROR, "linsolve returned an unexpected result", out_error);
        }

        boost::json::object result_obj;
        for (std::size_t i = 0; i < symbols.size(); ++i) {
            result_obj[symbols[i]->get_name()] = solution[i]->__str__();
        }
        const auto result = boost::json::serialize(result_obj);
        *out_result = copy_string(result.c_str());
        if (*out_result == nullptr) {
            return write_error(MATHSDK_STATUS_INTERNAL_ERROR, "Unable to allocate result", out_error);
        }
        return MATHSDK_STATUS_OK;
    } catch (SymEngine::SymEngineException &error) {
        return write_error(map_symengine_status(error.error_code()), error.what(), out_error);
    } catch (const std::exception &error) {
        return write_error(MATHSDK_STATUS_NOT_IMPLEMENTED, error.what(), out_error);
    } catch (...) {
        return write_error(MATHSDK_STATUS_INTERNAL_ERROR, "Unknown native error", out_error);
    }
}

MathSdkStatus math_sdk_limit(
    const char *expression,
    const char *variable,
    const char *target,
    int direction,
    char **out_result,
    char **out_error
) noexcept
{
    if (out_result == nullptr || out_error == nullptr) {
        return MATHSDK_STATUS_INVALID_ARGUMENT;
    }

    *out_result = nullptr;
    *out_error = nullptr;

    if (is_blank(expression) || is_blank(variable) || is_blank(target)) {
        return write_error(
            MATHSDK_STATUS_INVALID_ARGUMENT,
            "expression, variable, and target are required",
            out_error
        );
    }
    if (direction < -1 || direction > 1) {
        return write_error(
            MATHSDK_STATUS_INVALID_ARGUMENT,
            "direction must be -1, 0, or 1",
            out_error
        );
    }

    try {
        const auto parsed_expr = SymEngine::parse(expression);
        const auto parsed_var = SymEngine::parse(variable);
        if (!SymEngine::is_a<SymEngine::Symbol>(*parsed_var)) {
            return write_error(
                MATHSDK_STATUS_INVALID_ARGUMENT,
                "variable must be a single symbol, e.g. \"x\"",
                out_error
            );
        }
        const auto symbol = SymEngine::rcp_static_cast<const SymEngine::Symbol>(parsed_var);
        const auto target_value = SymEngine::parse(target);

        static const auto h = SymEngine::symbol("__mathsdk_limit_h");
        const SymEngine::map_basic_basic shift{{symbol, SymEngine::add(target_value, h)}};
        const auto shifted = SymEngine::subs(parsed_expr, shift);

        constexpr unsigned prec = 12;
        const auto series = SymEngine::UnivariateSeries::series(shifted, h->get_name(), prec);

        constexpr int min_order = -6;
        for (int order = min_order; order < static_cast<int>(prec); ++order) {
            const auto coeff = series->get_coeff(order);
            if (SymEngine::eq(*coeff, *SymEngine::zero)) {
                continue;
            }
            if (order == 0) {
                const auto result = coeff->__str__();
                *out_result = copy_string(result.c_str());
                return *out_result == nullptr
                    ? write_error(MATHSDK_STATUS_INTERNAL_ERROR, "Unable to allocate result", out_error)
                    : MATHSDK_STATUS_OK;
            }
            if (order > 0) {
                *out_result = copy_string("0");
                return *out_result == nullptr
                    ? write_error(MATHSDK_STATUS_INTERNAL_ERROR, "Unable to allocate result", out_error)
                    : MATHSDK_STATUS_OK;
            }

            if (direction == 0 && order % 2 != 0) {
                return write_error(
                    MATHSDK_STATUS_NOT_IMPLEMENTED,
                    "limit diverges with a sign that differs by side; request a one-sided limit",
                    out_error
                );
            }
            const auto coeff_sign = SymEngine::eval_double(*coeff) >= 0.0 ? 1 : -1;
            const auto side_sign = (order % 2 != 0 && direction == -1) ? -1 : 1;
            const auto is_positive = (coeff_sign * side_sign) > 0;
            *out_result = copy_string(is_positive ? "oo" : "-oo");
            return *out_result == nullptr
                ? write_error(MATHSDK_STATUS_INTERNAL_ERROR, "Unable to allocate result", out_error)
                : MATHSDK_STATUS_OK;
        }

        return write_error(
            MATHSDK_STATUS_NOT_IMPLEMENTED,
            "limit is zero to the computed precision, or the series could not "
            "isolate a leading term; this function only covers finite-order "
            "Taylor/Laurent expansions",
            out_error
        );
    } catch (SymEngine::SymEngineException &error) {
        return write_error(map_symengine_status(error.error_code()), error.what(), out_error);
    } catch (const std::exception &error) {
        return write_error(MATHSDK_STATUS_NOT_IMPLEMENTED, error.what(), out_error);
    } catch (...) {
        return write_error(MATHSDK_STATUS_INTERNAL_ERROR, "Unknown native error", out_error);
    }
}

MathSdkStatus math_sdk_integral(
    const char *expression,
    const char *variable,
    const char *lower_bound,
    const char *upper_bound,
    char **out_result,
    char **out_error
) noexcept
{
    if (out_result == nullptr || out_error == nullptr) {
        return MATHSDK_STATUS_INVALID_ARGUMENT;
    }

    *out_result = nullptr;
    *out_error = nullptr;

    if (is_blank(expression) || is_blank(variable)) {
        return write_error(
            MATHSDK_STATUS_INVALID_ARGUMENT,
            "expression and variable are required",
            out_error
        );
    }
    const bool has_lower = !is_blank(lower_bound);
    const bool has_upper = !is_blank(upper_bound);
    if (has_lower != has_upper) {
        return write_error(
            MATHSDK_STATUS_INVALID_ARGUMENT,
            "lower_bound and upper_bound must both be provided or both omitted",
            out_error
        );
    }

    try {
        const auto parsed_expr = SymEngine::parse(expression);
        const auto parsed_var = SymEngine::parse(variable);
        if (!SymEngine::is_a<SymEngine::Symbol>(*parsed_var)) {
            return write_error(
                MATHSDK_STATUS_INVALID_ARGUMENT,
                "variable must be a single symbol, e.g. \"x\"",
                out_error
            );
        }
        const auto symbol = SymEngine::rcp_static_cast<const SymEngine::Symbol>(parsed_var);

        const auto antiderivative = mathsdk::try_integrate(parsed_expr, symbol);
        if (antiderivative.is_null()) {
            return write_error(
                MATHSDK_STATUS_NOT_IMPLEMENTED,
                "No antiderivative found with the supported rule set "
                "(power rule, exp/log, sin/cos, linearity, linear-argument substitution)",
                out_error
            );
        }

        if (!has_lower) {
            const auto result = antiderivative->__str__() + " + C";
            *out_result = copy_string(result.c_str());
            return *out_result == nullptr
                ? write_error(MATHSDK_STATUS_INTERNAL_ERROR, "Unable to allocate result", out_error)
                : MATHSDK_STATUS_OK;
        }

        const auto lower = SymEngine::parse(lower_bound);
        const auto upper = SymEngine::parse(upper_bound);
        const auto lower_value = SymEngine::eval_double(*lower);
        const auto upper_value = SymEngine::eval_double(*upper);
        if (lower_value >= upper_value) {
            return write_error(
                MATHSDK_STATUS_INVALID_ARGUMENT,
                "lower_bound must be strictly less than upper_bound",
                out_error
            );
        }

        SymEngine::RCP<const SymEngine::Basic> numerator;
        SymEngine::RCP<const SymEngine::Basic> denominator;
        SymEngine::as_numer_denom(parsed_expr, SymEngine::outArg(numerator), SymEngine::outArg(denominator));
        if (!SymEngine::eq(*denominator, *SymEngine::one)) {
            const auto singularities = SymEngine::solve(denominator, symbol);
            if (SymEngine::is_a<SymEngine::FiniteSet>(*singularities)) {
                for (const auto &root : singularities->get_args()) {
                    try {
                        const auto root_value = SymEngine::eval_double(*root);
                        if (root_value >= lower_value && root_value <= upper_value) {
                            return write_error(
                                MATHSDK_STATUS_NOT_IMPLEMENTED,
                                "integrand has a singularity inside the interval",
                                out_error
                            );
                        }
                    } catch (const std::exception &) {
                    }
                }
            }
        }

        const SymEngine::map_basic_basic at_upper{{symbol, upper}};
        const SymEngine::map_basic_basic at_lower{{symbol, lower}};
        const auto value_at_upper = SymEngine::subs(antiderivative, at_upper);
        const auto value_at_lower = SymEngine::subs(antiderivative, at_lower);
        const auto definite_value = SymEngine::sub(value_at_upper, value_at_lower);

        if (SymEngine::eq(*definite_value, *SymEngine::ComplexInf)
            || SymEngine::eq(*definite_value, *SymEngine::Nan)) {
            return write_error(
                MATHSDK_STATUS_NOT_IMPLEMENTED,
                "definite integral is unbounded or undefined on this interval",
                out_error
            );
        }

        const auto result = definite_value->__str__();
        *out_result = copy_string(result.c_str());
        return *out_result == nullptr
            ? write_error(MATHSDK_STATUS_INTERNAL_ERROR, "Unable to allocate result", out_error)
            : MATHSDK_STATUS_OK;
    } catch (SymEngine::SymEngineException &error) {
        return write_error(map_symengine_status(error.error_code()), error.what(), out_error);
    } catch (const std::exception &error) {
        return write_error(MATHSDK_STATUS_NOT_IMPLEMENTED, error.what(), out_error);
    } catch (...) {
        return write_error(MATHSDK_STATUS_INTERNAL_ERROR, "Unknown native error", out_error);
    }
}

MathSdkStatus math_sdk_series(
    const char *expression,
    const char *variable,
    const char *around_point,
    int order,
    char **out_result,
    char **out_error
) noexcept
{
    if (out_result == nullptr || out_error == nullptr) {
        return MATHSDK_STATUS_INVALID_ARGUMENT;
    }

    *out_result = nullptr;
    *out_error = nullptr;

    if (is_blank(expression) || is_blank(variable) || is_blank(around_point)) {
        return write_error(
            MATHSDK_STATUS_INVALID_ARGUMENT,
            "expression, variable, and around_point are required",
            out_error
        );
    }
    if (order < 1 || order > 20) {
        return write_error(
            MATHSDK_STATUS_INVALID_ARGUMENT,
            "order must be between 1 and 20",
            out_error
        );
    }

    try {
        const auto parsed_expr = SymEngine::parse(expression);
        const auto parsed_var = SymEngine::parse(variable);
        if (!SymEngine::is_a<SymEngine::Symbol>(*parsed_var)) {
            return write_error(
                MATHSDK_STATUS_INVALID_ARGUMENT,
                "variable must be a single symbol, e.g. \"x\"",
                out_error
            );
        }
        const auto symbol = SymEngine::rcp_static_cast<const SymEngine::Symbol>(parsed_var);
        const auto point = SymEngine::parse(around_point);

        static const auto h = SymEngine::symbol("__mathsdk_series_h");
        const SymEngine::map_basic_basic shift{{symbol, SymEngine::add(point, h)}};
        const auto shifted = SymEngine::subs(parsed_expr, shift);

        const auto prec = static_cast<unsigned>(order) + 1;
        const auto series = SymEngine::UnivariateSeries::series(shifted, h->get_name(), prec);

        const SymEngine::RCP<const SymEngine::Basic> offset = SymEngine::eq(*point, *SymEngine::zero)
            ? SymEngine::RCP<const SymEngine::Basic>(symbol)
            : SymEngine::sub(symbol, point);

        SymEngine::RCP<const SymEngine::Basic> expansion = SymEngine::zero;
        for (const auto &[exponent, coefficient] : series->as_dict()) {
            if (exponent < 0 || exponent >= static_cast<int>(prec)) {
                continue;
            }
            const auto term = SymEngine::mul(coefficient, SymEngine::pow(offset, SymEngine::integer(exponent)));
            expansion = SymEngine::add(expansion, term);
        }

        const auto order_term = "O(" + offset->__str__() + "**" + std::to_string(order) + ")";
        const auto result = SymEngine::eq(*expansion, *SymEngine::zero)
            ? order_term
            : expansion->__str__() + " + " + order_term;

        *out_result = copy_string(result.c_str());
        return *out_result == nullptr
            ? write_error(MATHSDK_STATUS_INTERNAL_ERROR, "Unable to allocate result", out_error)
            : MATHSDK_STATUS_OK;
    } catch (SymEngine::SymEngineException &error) {
        return write_error(map_symengine_status(error.error_code()), error.what(), out_error);
    } catch (const std::exception &error) {
        return write_error(MATHSDK_STATUS_NOT_IMPLEMENTED, error.what(), out_error);
    } catch (...) {
        return write_error(MATHSDK_STATUS_INTERNAL_ERROR, "Unknown native error", out_error);
    }
}

MathSdkStatus math_sdk_matrix_op(
    const char *operation,
    const char *matrix_a,
    const char *matrix_b,
    char **out_result,
    char **out_error
) noexcept
{
    if (out_result == nullptr || out_error == nullptr) {
        return MATHSDK_STATUS_INVALID_ARGUMENT;
    }

    *out_result = nullptr;
    *out_error = nullptr;

    if (is_blank(operation) || is_blank(matrix_a)) {
        return write_error(
            MATHSDK_STATUS_INVALID_ARGUMENT,
            "operation and matrix_a are required",
            out_error
        );
    }

    const std::string op(operation);
    const bool needs_b = (op == "add" || op == "multiply");
    if (needs_b && is_blank(matrix_b)) {
        return write_error(
            MATHSDK_STATUS_INVALID_ARGUMENT,
            "matrix_b is required for \"add\" and \"multiply\"",
            out_error
        );
    }

    try {
        const auto a = parse_matrix(matrix_a);

        if (op == "determinant") {
            if (!a.is_square()) {
                return write_error(
                    MATHSDK_STATUS_INVALID_ARGUMENT,
                    "determinant requires a square matrix",
                    out_error
                );
            }
            const auto result = a.det()->__str__();
            *out_result = copy_string(result.c_str());
            return *out_result == nullptr
                ? write_error(MATHSDK_STATUS_INTERNAL_ERROR, "Unable to allocate result", out_error)
                : MATHSDK_STATUS_OK;
        }

        if (op == "inverse") {
            if (!a.is_square()) {
                return write_error(
                    MATHSDK_STATUS_INVALID_ARGUMENT,
                    "inverse requires a square matrix",
                    out_error
                );
            }
            if (SymEngine::eq(*a.det(), *SymEngine::zero)) {
                return write_error(
                    MATHSDK_STATUS_NOT_IMPLEMENTED,
                    "matrix is singular; no inverse exists",
                    out_error
                );
            }
            SymEngine::DenseMatrix result(a.nrows(), a.ncols());
            a.inv(result);
            const auto serialized = serialize_matrix(result);
            *out_result = copy_string(serialized.c_str());
            return *out_result == nullptr
                ? write_error(MATHSDK_STATUS_INTERNAL_ERROR, "Unable to allocate result", out_error)
                : MATHSDK_STATUS_OK;
        }

        if (op == "transpose") {
            SymEngine::DenseMatrix result(a.ncols(), a.nrows());
            a.transpose(result);
            const auto serialized = serialize_matrix(result);
            *out_result = copy_string(serialized.c_str());
            return *out_result == nullptr
                ? write_error(MATHSDK_STATUS_INTERNAL_ERROR, "Unable to allocate result", out_error)
                : MATHSDK_STATUS_OK;
        }

        if (op == "add") {
            const auto b = parse_matrix(matrix_b);
            if (a.nrows() != b.nrows() || a.ncols() != b.ncols()) {
                return write_error(
                    MATHSDK_STATUS_INVALID_ARGUMENT,
                    "add requires matrices of the same dimensions",
                    out_error
                );
            }
            SymEngine::DenseMatrix result(a.nrows(), a.ncols());
            a.add_matrix(b, result);
            const auto serialized = serialize_matrix(result);
            *out_result = copy_string(serialized.c_str());
            return *out_result == nullptr
                ? write_error(MATHSDK_STATUS_INTERNAL_ERROR, "Unable to allocate result", out_error)
                : MATHSDK_STATUS_OK;
        }

        if (op == "multiply") {
            const auto b = parse_matrix(matrix_b);
            if (a.ncols() != b.nrows()) {
                return write_error(
                    MATHSDK_STATUS_INVALID_ARGUMENT,
                    "multiply requires A.cols == B.rows",
                    out_error
                );
            }
            SymEngine::DenseMatrix result(a.nrows(), b.ncols());
            a.mul_matrix(b, result);
            const auto serialized = serialize_matrix(result);
            *out_result = copy_string(serialized.c_str());
            return *out_result == nullptr
                ? write_error(MATHSDK_STATUS_INTERNAL_ERROR, "Unable to allocate result", out_error)
                : MATHSDK_STATUS_OK;
        }

        return write_error(
            MATHSDK_STATUS_INVALID_ARGUMENT,
            "operation must be one of: determinant, inverse, transpose, add, multiply",
            out_error
        );
    } catch (const std::invalid_argument &error) {
        return write_error(MATHSDK_STATUS_INVALID_ARGUMENT, error.what(), out_error);
    } catch (SymEngine::SymEngineException &error) {
        return write_error(map_symengine_status(error.error_code()), error.what(), out_error);
    } catch (const std::exception &error) {
        return write_error(MATHSDK_STATUS_NOT_IMPLEMENTED, error.what(), out_error);
    } catch (...) {
        return write_error(MATHSDK_STATUS_INTERNAL_ERROR, "Unknown native error", out_error);
    }
}

MathSdkStatus math_sdk_ode_solve_separable(
    const char *f_of_x,
    const char *g_of_y,
    const char *x_variable,
    const char *y_variable,
    char **out_result,
    char **out_error
) noexcept
{
    if (out_result == nullptr || out_error == nullptr) {
        return MATHSDK_STATUS_INVALID_ARGUMENT;
    }

    *out_result = nullptr;
    *out_error = nullptr;

    if (is_blank(f_of_x) || is_blank(g_of_y) || is_blank(x_variable) || is_blank(y_variable)) {
        return write_error(
            MATHSDK_STATUS_INVALID_ARGUMENT,
            "f_of_x, g_of_y, x_variable, and y_variable are all required",
            out_error
        );
    }

    try {
        const auto parsed_x = SymEngine::parse(x_variable);
        const auto parsed_y = SymEngine::parse(y_variable);
        if (!SymEngine::is_a<SymEngine::Symbol>(*parsed_x)
            || !SymEngine::is_a<SymEngine::Symbol>(*parsed_y)) {
            return write_error(
                MATHSDK_STATUS_INVALID_ARGUMENT,
                "x_variable and y_variable must each be a single symbol",
                out_error
            );
        }
        const auto x = SymEngine::rcp_static_cast<const SymEngine::Symbol>(parsed_x);
        const auto y = SymEngine::rcp_static_cast<const SymEngine::Symbol>(parsed_y);
        if (SymEngine::eq(*x, *y)) {
            return write_error(
                MATHSDK_STATUS_INVALID_ARGUMENT,
                "x_variable and y_variable must be different symbols",
                out_error
            );
        }

        const auto parsed_g = SymEngine::parse(g_of_y);
        if (SymEngine::eq(*parsed_g, *SymEngine::zero)) {
            return write_error(
                MATHSDK_STATUS_DIVISION_BY_ZERO,
                "g_of_y cannot be identically zero",
                out_error
            );
        }
        const auto inverse_g = SymEngine::div(SymEngine::one, parsed_g);
        const auto y_side = mathsdk::try_integrate(inverse_g, y);
        if (y_side.is_null()) {
            return write_error(
                MATHSDK_STATUS_NOT_IMPLEMENTED,
                "could not integrate 1/g_of_y with respect to y_variable "
                "using the supported rule set",
                out_error
            );
        }

        const auto parsed_f = SymEngine::parse(f_of_x);
        const auto x_side = mathsdk::try_integrate(parsed_f, x);
        if (x_side.is_null()) {
            return write_error(
                MATHSDK_STATUS_NOT_IMPLEMENTED,
                "could not integrate f_of_x with respect to x_variable "
                "using the supported rule set",
                out_error
            );
        }

        const auto result = y_side->__str__() + " = " + x_side->__str__() + " + C";
        *out_result = copy_string(result.c_str());
        return *out_result == nullptr
            ? write_error(MATHSDK_STATUS_INTERNAL_ERROR, "Unable to allocate result", out_error)
            : MATHSDK_STATUS_OK;
    } catch (SymEngine::SymEngineException &error) {
        return write_error(map_symengine_status(error.error_code()), error.what(), out_error);
    } catch (const std::exception &error) {
        return write_error(MATHSDK_STATUS_NOT_IMPLEMENTED, error.what(), out_error);
    } catch (...) {
        return write_error(MATHSDK_STATUS_INTERNAL_ERROR, "Unknown native error", out_error);
    }
}

void math_sdk_free_string(char *value) noexcept
{
    std::free(value);
}
