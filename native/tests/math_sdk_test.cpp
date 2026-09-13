#include "mathsdk/math_sdk.h"

#include <cmath>
#include <cstring>
#include <initializer_list>
#include <iostream>
#include <string>

namespace
{

int failures = 0;

void check(bool condition, const std::string &message)
{
    if (condition) {
        return;
    }

    std::cerr << "FAIL: " << message << '\n';
    ++failures;
}

void expect_result(const char *expression, const char *expected)
{
    char *result = nullptr;
    char *error = nullptr;
    const auto status = math_sdk_evaluate(expression, &result, &error);

    check(status == MATHSDK_STATUS_OK, std::string(expression) + " status");
    check(error == nullptr, std::string(expression) + " error");
    check(result != nullptr, std::string(expression) + " result allocation");
    if (result != nullptr) {
        check(std::strcmp(result, expected) == 0, std::string(expression) + " value");
    }

    math_sdk_free_string(result);
    math_sdk_free_string(error);
}

void expect_error(const char *expression, MathSdkStatus expected)
{
    char *result = nullptr;
    char *error = nullptr;
    const auto status = math_sdk_evaluate(expression, &result, &error);

    check(status == expected, "error status");
    check(result == nullptr, "error result");
    check(error != nullptr, "error message");

    math_sdk_free_string(result);
    math_sdk_free_string(error);
}

void expect_steps(
    const char *expression,
    const char *expected_result,
    int expected_step_count
)
{
    char *result = nullptr;
    char *steps = nullptr;
    char *error = nullptr;
    const auto status = math_sdk_evaluate_steps(expression, &result, &steps, &error);

    check(status == MATHSDK_STATUS_OK, std::string(expression) + " steps status");
    check(error == nullptr, std::string(expression) + " steps error");
    if (result != nullptr) {
        check(
            std::strcmp(result, expected_result) == 0,
            std::string(expression) + " steps result value"
        );
    } else {
        check(false, std::string(expression) + " steps result allocation");
    }
    if (steps != nullptr) {
        const std::string steps_str(steps);
        int rule_count = 0;
        std::size_t pos = 0;
        while ((pos = steps_str.find("\"rule\"", pos)) != std::string::npos) {
            ++rule_count;
            pos += 6;
        }
        check(
            rule_count == expected_step_count,
            std::string(expression) + " steps count (expected "
                + std::to_string(expected_step_count) + ", got "
                + std::to_string(rule_count) + "): " + steps_str
        );
        check(steps_str.front() == '[' && steps_str.back() == ']',
            std::string(expression) + " steps is a JSON array");
    } else {
        check(false, std::string(expression) + " steps allocation");
    }

    math_sdk_free_string(result);
    math_sdk_free_string(steps);
    math_sdk_free_string(error);
}

void expect_steps_error(const char *expression, MathSdkStatus expected)
{
    char *result = nullptr;
    char *steps = nullptr;
    char *error = nullptr;
    const auto status = math_sdk_evaluate_steps(expression, &result, &steps, &error);

    check(status == expected, std::string(expression) + " steps error status");
    check(result == nullptr, std::string(expression) + " steps error result");
    check(steps == nullptr, std::string(expression) + " steps error steps");
    check(error != nullptr, std::string(expression) + " steps error message");

    math_sdk_free_string(result);
    math_sdk_free_string(steps);
    math_sdk_free_string(error);
}

void expect_substitute(
    const char *expression,
    const char *variable,
    const char *value,
    const char *expected
)
{
    char *result = nullptr;
    char *error = nullptr;
    const auto status = math_sdk_substitute(expression, variable, value, &result, &error);

    check(status == MATHSDK_STATUS_OK, std::string(expression) + " substitute status");
    check(error == nullptr, std::string(expression) + " substitute error");
    if (result != nullptr) {
        check(std::strcmp(result, expected) == 0, std::string(expression) + " substitute value");
    } else {
        check(false, std::string(expression) + " substitute result allocation");
    }

    math_sdk_free_string(result);
    math_sdk_free_string(error);
}

void expect_substitute_error(
    const char *expression,
    const char *variable,
    const char *value,
    MathSdkStatus expected
)
{
    char *result = nullptr;
    char *error = nullptr;
    const auto status = math_sdk_substitute(expression, variable, value, &result, &error);

    check(status == expected, std::string(expression) + " substitute error status");
    check(result == nullptr, std::string(expression) + " substitute error result");
    check(error != nullptr, std::string(expression) + " substitute error message");

    math_sdk_free_string(result);
    math_sdk_free_string(error);
}

void expect_numeric(const char *expression, double expected)
{
    double result = 0.0;
    char *error = nullptr;
    const auto status = math_sdk_evaluate_numeric(expression, &result, &error);

    check(status == MATHSDK_STATUS_OK, std::string(expression) + " numeric status");
    check(error == nullptr, std::string(expression) + " numeric error");
    check(std::abs(result - expected) < 1e-9, std::string(expression) + " numeric value");

    math_sdk_free_string(error);
}

void expect_numeric_error(const char *expression, MathSdkStatus expected)
{
    double result = 0.0;
    char *error = nullptr;
    const auto status = math_sdk_evaluate_numeric(expression, &result, &error);

    check(status == expected, std::string(expression) + " numeric error status");
    check(error != nullptr, std::string(expression) + " numeric error message");

    math_sdk_free_string(error);
}

void expect_derivative(
    const char *expression,
    const char *variable,
    int order,
    const char *expected
)
{
    char *result = nullptr;
    char *error = nullptr;
    const auto status = math_sdk_derivative(expression, variable, order, &result, &error);

    check(status == MATHSDK_STATUS_OK, std::string(expression) + " derivative status");
    check(error == nullptr, std::string(expression) + " derivative error");
    if (result != nullptr) {
        check(std::strcmp(result, expected) == 0, std::string(expression) + " derivative value");
    } else {
        check(false, std::string(expression) + " derivative result allocation");
    }

    math_sdk_free_string(result);
    math_sdk_free_string(error);
}

void expect_derivative_error(
    const char *expression,
    const char *variable,
    int order,
    MathSdkStatus expected
)
{
    char *result = nullptr;
    char *error = nullptr;
    const auto status = math_sdk_derivative(expression, variable, order, &result, &error);

    check(status == expected, std::string(expression) + " derivative error status");
    check(result == nullptr, std::string(expression) + " derivative error result");
    check(error != nullptr, std::string(expression) + " derivative error message");

    math_sdk_free_string(result);
    math_sdk_free_string(error);
}

void expect_solve_contains(
    const char *equation,
    const char *variable,
    std::initializer_list<const char *> expected_roots
)
{
    char *result = nullptr;
    char *error = nullptr;
    const auto status = math_sdk_solve(equation, variable, &result, &error);

    check(status == MATHSDK_STATUS_OK, std::string(equation) + " solve status");
    check(error == nullptr, std::string(equation) + " solve error");
    if (result != nullptr) {
        const std::string result_str(result);
        for (const auto *root : expected_roots) {
            check(
                result_str.find(root) != std::string::npos,
                std::string(equation) + " solve contains " + root + ": " + result_str
            );
        }
    } else {
        check(false, std::string(equation) + " solve result allocation");
    }

    math_sdk_free_string(result);
    math_sdk_free_string(error);
}

void expect_solve_error(
    const char *equation,
    const char *variable,
    MathSdkStatus expected
)
{
    char *result = nullptr;
    char *error = nullptr;
    const auto status = math_sdk_solve(equation, variable, &result, &error);

    check(status == expected, std::string(equation) + " solve error status");
    check(error != nullptr, std::string(equation) + " solve error message");

    math_sdk_free_string(result);
    math_sdk_free_string(error);
}

void expect_linear_system_contains(
    const char *equations,
    const char *variables,
    std::initializer_list<const char *> expected_fragments
)
{
    char *result = nullptr;
    char *error = nullptr;
    const auto status = math_sdk_solve_linear_system(equations, variables, &result, &error);

    check(status == MATHSDK_STATUS_OK, std::string(equations) + " linear system status");
    check(error == nullptr, std::string(equations) + " linear system error");
    if (result != nullptr) {
        const std::string result_str(result);
        for (const auto *fragment : expected_fragments) {
            check(
                result_str.find(fragment) != std::string::npos,
                std::string(equations) + " linear system contains " + fragment + ": " + result_str
            );
        }
    } else {
        check(false, std::string(equations) + " linear system result allocation");
    }

    math_sdk_free_string(result);
    math_sdk_free_string(error);
}

void expect_linear_system_error(
    const char *equations,
    const char *variables,
    MathSdkStatus expected
)
{
    char *result = nullptr;
    char *error = nullptr;
    const auto status = math_sdk_solve_linear_system(equations, variables, &result, &error);

    check(status == expected, std::string(equations) + " linear system error status");
    check(error != nullptr, std::string(equations) + " linear system error message");

    math_sdk_free_string(result);
    math_sdk_free_string(error);
}

void expect_limit(
    const char *expression,
    const char *variable,
    const char *target,
    int direction,
    const char *expected
)
{
    char *result = nullptr;
    char *error = nullptr;
    const auto status = math_sdk_limit(expression, variable, target, direction, &result, &error);

    check(status == MATHSDK_STATUS_OK, std::string(expression) + " limit status");
    check(error == nullptr, std::string(expression) + " limit error");
    if (result != nullptr) {
        check(std::strcmp(result, expected) == 0, std::string(expression) + " limit value");
    } else {
        check(false, std::string(expression) + " limit result allocation");
    }

    math_sdk_free_string(result);
    math_sdk_free_string(error);
}

void expect_limit_error(
    const char *expression,
    const char *variable,
    const char *target,
    int direction,
    MathSdkStatus expected
)
{
    char *result = nullptr;
    char *error = nullptr;
    const auto status = math_sdk_limit(expression, variable, target, direction, &result, &error);

    check(status == expected, std::string(expression) + " limit error status");
    check(error != nullptr, std::string(expression) + " limit error message");

    math_sdk_free_string(result);
    math_sdk_free_string(error);
}

void expect_integral(
    const char *expression,
    const char *variable,
    const char *lower,
    const char *upper,
    const char *expected
)
{
    char *result = nullptr;
    char *error = nullptr;
    const auto status = math_sdk_integral(expression, variable, lower, upper, &result, &error);

    check(status == MATHSDK_STATUS_OK, std::string(expression) + " integral status");
    check(error == nullptr, std::string(expression) + " integral error");
    if (result != nullptr) {
        check(
            std::strcmp(result, expected) == 0,
            std::string(expression) + " integral value: got [" + result
                + "] expected [" + expected + "]"
        );
    } else {
        check(false, std::string(expression) + " integral result allocation");
    }

    math_sdk_free_string(result);
    math_sdk_free_string(error);
}

void expect_integral_error(
    const char *expression,
    const char *variable,
    const char *lower,
    const char *upper,
    MathSdkStatus expected
)
{
    char *result = nullptr;
    char *error = nullptr;
    const auto status = math_sdk_integral(expression, variable, lower, upper, &result, &error);

    check(status == expected, std::string(expression) + " integral error status");
    check(error != nullptr, std::string(expression) + " integral error message");

    math_sdk_free_string(result);
    math_sdk_free_string(error);
}

void expect_series_contains(
    const char *expression,
    const char *variable,
    const char *around_point,
    int order,
    std::initializer_list<const char *> expected_fragments
)
{
    char *result = nullptr;
    char *error = nullptr;
    const auto status = math_sdk_series(expression, variable, around_point, order, &result, &error);

    check(status == MATHSDK_STATUS_OK, std::string(expression) + " series status");
    check(error == nullptr, std::string(expression) + " series error");
    if (result != nullptr) {
        const std::string result_str(result);
        for (const auto *fragment : expected_fragments) {
            check(
                result_str.find(fragment) != std::string::npos,
                std::string(expression) + " series contains " + fragment + ": " + result_str
            );
        }
    } else {
        check(false, std::string(expression) + " series result allocation");
    }

    math_sdk_free_string(result);
    math_sdk_free_string(error);
}

void expect_series_error(
    const char *expression,
    const char *variable,
    const char *around_point,
    int order,
    MathSdkStatus expected
)
{
    char *result = nullptr;
    char *error = nullptr;
    const auto status = math_sdk_series(expression, variable, around_point, order, &result, &error);

    check(status == expected, std::string(expression) + " series error status");
    check(error != nullptr, std::string(expression) + " series error message");

    math_sdk_free_string(result);
    math_sdk_free_string(error);
}

void expect_matrix(
    const char *operation,
    const char *matrix_a,
    const char *matrix_b,
    const char *expected
)
{
    char *result = nullptr;
    char *error = nullptr;
    const auto status = math_sdk_matrix_op(operation, matrix_a, matrix_b, &result, &error);

    check(status == MATHSDK_STATUS_OK, std::string(operation) + " matrix status");
    check(error == nullptr, std::string(operation) + " matrix error");
    if (result != nullptr) {
        check(
            std::strcmp(result, expected) == 0,
            std::string(operation) + " matrix value: got [" + result + "] expected [" + expected + "]"
        );
    } else {
        check(false, std::string(operation) + " matrix result allocation");
    }

    math_sdk_free_string(result);
    math_sdk_free_string(error);
}

void expect_matrix_error(
    const char *operation,
    const char *matrix_a,
    const char *matrix_b,
    MathSdkStatus expected
)
{
    char *result = nullptr;
    char *error = nullptr;
    const auto status = math_sdk_matrix_op(operation, matrix_a, matrix_b, &result, &error);

    check(status == expected, std::string(operation) + " matrix error status");
    check(error != nullptr, std::string(operation) + " matrix error message");

    math_sdk_free_string(result);
    math_sdk_free_string(error);
}

void expect_ode(
    const char *f_of_x,
    const char *g_of_y,
    const char *x_variable,
    const char *y_variable,
    const char *expected
)
{
    char *result = nullptr;
    char *error = nullptr;
    const auto status = math_sdk_ode_solve_separable(
        f_of_x, g_of_y, x_variable, y_variable, &result, &error
    );

    check(status == MATHSDK_STATUS_OK, std::string(f_of_x) + " ode status");
    check(error == nullptr, std::string(f_of_x) + " ode error");
    if (result != nullptr) {
        check(
            std::strcmp(result, expected) == 0,
            std::string(f_of_x) + " ode value: got [" + result + "] expected [" + expected + "]"
        );
    } else {
        check(false, std::string(f_of_x) + " ode result allocation");
    }

    math_sdk_free_string(result);
    math_sdk_free_string(error);
}

void expect_ode_error(
    const char *f_of_x,
    const char *g_of_y,
    const char *x_variable,
    const char *y_variable,
    MathSdkStatus expected
)
{
    char *result = nullptr;
    char *error = nullptr;
    const auto status = math_sdk_ode_solve_separable(
        f_of_x, g_of_y, x_variable, y_variable, &result, &error
    );

    check(status == expected, std::string(f_of_x) + " ode error status");
    check(error != nullptr, std::string(f_of_x) + " ode error message");

    math_sdk_free_string(result);
    math_sdk_free_string(error);
}

} // namespace

int main()
{
    check(std::strcmp(math_sdk_version(), "0.1.0") == 0, "SDK version");
    expect_result("2 + 3 * 4", "14");
    expect_result("1/3 + 1/6", "1/2");
    expect_result("2*x + 3*x", "5*x");
    expect_result("2^10", "1024");
    expect_result("sqrt(16)", "4");
    expect_error("", MATHSDK_STATUS_INVALID_ARGUMENT);
    expect_error("2 +", MATHSDK_STATUS_PARSE_ERROR);
    expect_error("1/0", MATHSDK_STATUS_DIVISION_BY_ZERO);
    expect_error("0/0", MATHSDK_STATUS_DOMAIN_ERROR);

    char *error = nullptr;
    check(
        math_sdk_evaluate("1", nullptr, &error)
            == MATHSDK_STATUS_INVALID_ARGUMENT,
        "null result pointer"
    );
    math_sdk_free_string(error);


    expect_steps("2 + 3 * 4", "14", 2);
    expect_steps("1/3 + 1/6", "1/2", 3);
    expect_steps("2^10", "1024", 1);
    expect_steps("sqrt(16)", "4", 1);
    expect_steps("(2 + 3) * 4", "20", 2);
    expect_steps("7", "7", 1);
    expect_steps_error(
        "1/3037000500 + 1/3037000501", MATHSDK_STATUS_NOT_IMPLEMENTED
    );

    expect_steps_error("", MATHSDK_STATUS_INVALID_ARGUMENT);
    expect_steps_error("2 +", MATHSDK_STATUS_PARSE_ERROR);
    expect_steps_error("1/0", MATHSDK_STATUS_DIVISION_BY_ZERO);
    expect_steps_error("sqrt(2)", MATHSDK_STATUS_NOT_IMPLEMENTED);

    char *steps_null_check_error = nullptr;
    check(
        math_sdk_evaluate_steps("1", nullptr, nullptr, &steps_null_check_error)
            == MATHSDK_STATUS_INVALID_ARGUMENT,
        "evaluate_steps null result pointer"
    );
    math_sdk_free_string(steps_null_check_error);

    expect_substitute("x^2 + 2*x + 1", "x", "3", "16");
    {
        char *substituted = nullptr;
        char *canonical = nullptr;
        char *error = nullptr;
        math_sdk_substitute("x + y", "x", "5", &substituted, &error);
        math_sdk_evaluate("5 + y", &canonical, &error);
        check(
            substituted != nullptr && canonical != nullptr
                && std::strcmp(substituted, canonical) == 0,
            "x + y substitute matches evaluate(5 + y)"
        );
        math_sdk_free_string(substituted);
        math_sdk_free_string(canonical);
        math_sdk_free_string(error);
    }
    expect_substitute_error("x", "x", "1/0", MATHSDK_STATUS_DIVISION_BY_ZERO);
    expect_substitute_error("x", "2", "1", MATHSDK_STATUS_INVALID_ARGUMENT);
    expect_substitute_error("", "x", "1", MATHSDK_STATUS_INVALID_ARGUMENT);

    expect_numeric("1/4", 0.25);
    expect_numeric("2^10", 1024.0);
    expect_numeric("sqrt(2)", 1.4142135623730951);
    expect_numeric_error("", MATHSDK_STATUS_INVALID_ARGUMENT);
    expect_numeric_error("2 +", MATHSDK_STATUS_PARSE_ERROR);
    expect_numeric_error("1/0", MATHSDK_STATUS_DIVISION_BY_ZERO);
    expect_numeric_error("x + 1", MATHSDK_STATUS_NOT_IMPLEMENTED);

    {
        char *derived = nullptr;
        char *canonical = nullptr;
        char *error = nullptr;
        math_sdk_derivative("x^2 + 2*x", "x", 1, &derived, &error);
        math_sdk_evaluate("2*x + 2", &canonical, &error);
        check(
            derived != nullptr && canonical != nullptr
                && std::strcmp(derived, canonical) == 0,
            "x^2 + 2*x derivative matches evaluate(2*x + 2)"
        );
        math_sdk_free_string(derived);
        math_sdk_free_string(canonical);
        math_sdk_free_string(error);
    }
    expect_derivative("x^3", "x", 2, "6*x");
    expect_derivative("5", "x", 1, "0");
    expect_derivative_error("x^2", "2", 1, MATHSDK_STATUS_INVALID_ARGUMENT);
    expect_derivative_error("x^2", "x", 0, MATHSDK_STATUS_INVALID_ARGUMENT);
    expect_derivative_error("x^2", "x", 11, MATHSDK_STATUS_INVALID_ARGUMENT);
    expect_derivative_error("", "x", 1, MATHSDK_STATUS_INVALID_ARGUMENT);

    expect_solve_contains("x^2 - 4 = 0", "x", {"-2", "2"});
    expect_solve_contains("x - 5 = 0", "x", {"5"});
    expect_solve_contains("x^2 + 1 = 0", "x", {"I"});
    expect_solve_error("x^2", "2", MATHSDK_STATUS_INVALID_ARGUMENT);
    expect_solve_error("", "x", MATHSDK_STATUS_INVALID_ARGUMENT);
    expect_solve_error("sin(x) = 0", "x", MATHSDK_STATUS_NOT_IMPLEMENTED);

    expect_linear_system_contains("2*x + y = 5;x - y = 1", "x,y", {"\"x\":\"2\"", "\"y\":\"1\""});
    expect_linear_system_error("x + y = 1;x + y = 2", "x,y", MATHSDK_STATUS_NOT_IMPLEMENTED);
    expect_linear_system_error("x + y = 1", "x,y", MATHSDK_STATUS_INVALID_ARGUMENT);
    expect_linear_system_error("", "x", MATHSDK_STATUS_INVALID_ARGUMENT);

    expect_limit("x + 1", "x", "2", 0, "3");
    expect_limit("sin(x)/x", "x", "0", 0, "1");
    expect_limit("1/x^2", "x", "0", 0, "oo");
    expect_limit("1/x", "x", "0", 1, "oo");
    expect_limit("1/x", "x", "0", -1, "-oo");
    expect_limit_error("1/x", "x", "0", 0, MATHSDK_STATUS_NOT_IMPLEMENTED);
    expect_limit_error("x", "2", "0", 0, MATHSDK_STATUS_INVALID_ARGUMENT);
    expect_limit_error("", "x", "0", 0, MATHSDK_STATUS_INVALID_ARGUMENT);
    expect_limit_error("x", "x", "0", 2, MATHSDK_STATUS_INVALID_ARGUMENT);

    expect_integral("x^2", "x", nullptr, nullptr, "(1/3)*x**3 + C");
    expect_integral("2*x + 3", "x", nullptr, nullptr, "3*x + x**2 + C");
    expect_integral("cos(2*x)", "x", nullptr, nullptr, "(1/2)*sin(2*x) + C");
    expect_integral("x^2", "x", "0", "3", "9");
    expect_integral("1/x", "x", "1", "2", "log(2)");
    expect_integral_error("1/x^2", "x", "-1", "1", MATHSDK_STATUS_NOT_IMPLEMENTED);
    expect_integral_error("1/(x-5)", "x", "0", nullptr, MATHSDK_STATUS_INVALID_ARGUMENT);
    expect_integral_error("1/x", "x", "0", "1", MATHSDK_STATUS_NOT_IMPLEMENTED);
    expect_integral_error("1/x", "x", "-1", "1", MATHSDK_STATUS_NOT_IMPLEMENTED);
    expect_integral_error("tan(x)", "x", nullptr, nullptr, MATHSDK_STATUS_NOT_IMPLEMENTED);
    expect_integral_error("x*sin(x)", "x", nullptr, nullptr, MATHSDK_STATUS_NOT_IMPLEMENTED);
    expect_integral_error("", "x", nullptr, nullptr, MATHSDK_STATUS_INVALID_ARGUMENT);
    expect_integral_error("x^2", "2", nullptr, nullptr, MATHSDK_STATUS_INVALID_ARGUMENT);

    expect_series_contains("sin(x)", "x", "0", 4, {"x", "x**3"});
    expect_series_contains("1/(1-x)", "x", "0", 3, {"1", "x", "x**2"});
    expect_series_contains("x^2 + 1", "x", "0", 3, {"x**2", "1"});
    expect_series_error("x", "2", "0", 3, MATHSDK_STATUS_INVALID_ARGUMENT);
    expect_series_error("x", "x", "0", 0, MATHSDK_STATUS_INVALID_ARGUMENT);
    expect_series_error("x", "x", "0", 21, MATHSDK_STATUS_INVALID_ARGUMENT);
    expect_series_error("", "x", "0", 3, MATHSDK_STATUS_INVALID_ARGUMENT);

    expect_matrix("determinant", "2;2;1,2,3,4", nullptr, "-2");
    expect_matrix("transpose", "2;2;1,2,3,4", nullptr, "2;2;1,3,2,4");
    expect_matrix("add", "2;2;1,2,3,4", "2;2;5,6,7,8", "2;2;6,8,10,12");
    expect_matrix("multiply", "2;2;1,0,0,1", "2;2;5,6,7,8", "2;2;5,6,7,8");
    {
        char *inverse = nullptr;
        char *error = nullptr;
        math_sdk_matrix_op("inverse", "2;2;1,2,3,4", nullptr, &inverse, &error);
        check(inverse != nullptr, "inverse matrix result allocation");
        if (inverse != nullptr) {
            char *identity_check = nullptr;
            math_sdk_matrix_op("multiply", "2;2;1,2,3,4", inverse, &identity_check, &error);
            check(
                identity_check != nullptr
                    && std::strcmp(identity_check, "2;2;1,0,0,1") == 0,
                std::string("inverse * original == identity: ")
                    + (identity_check ? identity_check : "(null)")
            );
            math_sdk_free_string(identity_check);
        }
        math_sdk_free_string(inverse);
        math_sdk_free_string(error);
    }
    expect_matrix_error("inverse", "2;2;1,2,2,4", nullptr, MATHSDK_STATUS_NOT_IMPLEMENTED);
    expect_matrix_error("determinant", "2;3;1,2,3,4,5,6", nullptr, MATHSDK_STATUS_INVALID_ARGUMENT);
    expect_matrix_error("add", "2;2;1,2,3,4", "3;3;1,2,3,4,5,6,7,8,9", MATHSDK_STATUS_INVALID_ARGUMENT);
    expect_matrix_error("multiply", "2;2;1,2,3,4", "3;2;1,2,3,4,5,6", MATHSDK_STATUS_INVALID_ARGUMENT);
    expect_matrix_error("bogus", "2;2;1,2,3,4", nullptr, MATHSDK_STATUS_INVALID_ARGUMENT);
    expect_matrix_error("determinant", "", nullptr, MATHSDK_STATUS_INVALID_ARGUMENT);
    expect_matrix_error(
        "determinant", "99999999999999999999;2;1,2,3,4", nullptr,
        MATHSDK_STATUS_INVALID_ARGUMENT
    );
    expect_matrix_error("determinant", "abc;2;1,2,3,4", nullptr, MATHSDK_STATUS_INVALID_ARGUMENT);

    expect_ode("x", "y", "x", "y", "log(y) = (1/2)*x**2 + C");
    expect_ode("x", "1", "x", "y", "y = (1/2)*x**2 + C");
    expect_ode_error("tan(x)", "1", "x", "y", MATHSDK_STATUS_NOT_IMPLEMENTED);
    expect_ode_error("x", "0", "x", "y", MATHSDK_STATUS_DIVISION_BY_ZERO);
    expect_ode_error("x", "y", "x", "x", MATHSDK_STATUS_INVALID_ARGUMENT);
    expect_ode_error("x", "y", "2", "y", MATHSDK_STATUS_INVALID_ARGUMENT);
    expect_ode_error("", "y", "x", "y", MATHSDK_STATUS_INVALID_ARGUMENT);

    if (failures == 0) {
        std::cout << "MathSDK native tests passed\n";
    }
    return failures == 0 ? 0 : 1;
}
