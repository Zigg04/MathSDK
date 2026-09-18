#ifndef MATHSDK_MATH_SDK_H
#define MATHSDK_MATH_SDK_H

#if defined(_WIN32)
#if defined(MATHSDK_BUILD)
#define MATHSDK_API __declspec(dllexport)
#else
#define MATHSDK_API __declspec(dllimport)
#endif
#elif defined(__GNUC__)
#define MATHSDK_API __attribute__((visibility("default")))
#else
#define MATHSDK_API
#endif

#ifdef __cplusplus
#define MATHSDK_NOEXCEPT noexcept
extern "C" {
#else
#define MATHSDK_NOEXCEPT
#endif

/**
 * Result code returned by every MathSDK entry point.
 *
 * `MATHSDK_STATUS_OK` means the call wrote `out_result`. Every other value
 * means it wrote `out_error` instead and left `out_result` untouched.
 * `MATHSDK_STATUS_NOT_IMPLEMENTED` means the input is outside this version's
 * supported rule set, not that the call failed.
 */
typedef enum MathSdkStatus {
    /** The call succeeded and wrote `out_result`. */
    MATHSDK_STATUS_OK = 0,
    /** A required argument was null, empty, or malformed. */
    MATHSDK_STATUS_INVALID_ARGUMENT = 1,
    /** The expression could not be parsed. */
    MATHSDK_STATUS_PARSE_ERROR = 2,
    /** The computation required a division by zero. */
    MATHSDK_STATUS_DIVISION_BY_ZERO = 3,
    /** The input is outside the mathematical domain of the operation. */
    MATHSDK_STATUS_DOMAIN_ERROR = 4,
    /** The input is outside this version's supported rule set. */
    MATHSDK_STATUS_NOT_IMPLEMENTED = 5,
    /** An unexpected internal failure. Report this as a bug. */
    MATHSDK_STATUS_INTERNAL_ERROR = 100
} MathSdkStatus;

MATHSDK_API const char *math_sdk_version(void) MATHSDK_NOEXCEPT;

MATHSDK_API MathSdkStatus math_sdk_evaluate(
    const char *expression,
    char **out_result,
    char **out_error
) MATHSDK_NOEXCEPT;


MATHSDK_API MathSdkStatus math_sdk_evaluate_steps(
    const char *expression,
    char **out_result,
    char **out_steps,
    char **out_error
) MATHSDK_NOEXCEPT;


MATHSDK_API MathSdkStatus math_sdk_substitute(
    const char *expression,
    const char *variable,
    const char *value,
    char **out_result,
    char **out_error
) MATHSDK_NOEXCEPT;

MATHSDK_API MathSdkStatus math_sdk_evaluate_numeric(
    const char *expression,
    double *out_result,
    char **out_error
) MATHSDK_NOEXCEPT;

MATHSDK_API MathSdkStatus math_sdk_derivative(
    const char *expression,
    const char *variable,
    int order,
    char **out_result,
    char **out_error
) MATHSDK_NOEXCEPT;

MATHSDK_API MathSdkStatus math_sdk_solve(
    const char *equation,
    const char *variable,
    char **out_result,
    char **out_error
) MATHSDK_NOEXCEPT;

MATHSDK_API MathSdkStatus math_sdk_solve_linear_system(
    const char *equations,
    const char *variables,
    char **out_result,
    char **out_error
) MATHSDK_NOEXCEPT;

MATHSDK_API MathSdkStatus math_sdk_limit(
    const char *expression,
    const char *variable,
    const char *target,
    int direction,
    char **out_result,
    char **out_error
) MATHSDK_NOEXCEPT;


MATHSDK_API MathSdkStatus math_sdk_integral(
    const char *expression,
    const char *variable,
    const char *lower_bound,
    const char *upper_bound,
    char **out_result,
    char **out_error
) MATHSDK_NOEXCEPT;


MATHSDK_API MathSdkStatus math_sdk_series(
    const char *expression,
    const char *variable,
    const char *around_point,
    int order,
    char **out_result,
    char **out_error
) MATHSDK_NOEXCEPT;


MATHSDK_API MathSdkStatus math_sdk_matrix_op(
    const char *operation,
    const char *matrix_a,
    const char *matrix_b,
    char **out_result,
    char **out_error
) MATHSDK_NOEXCEPT;


MATHSDK_API MathSdkStatus math_sdk_ode_solve_separable(
    const char *f_of_x,
    const char *g_of_y,
    const char *x_variable,
    const char *y_variable,
    char **out_result,
    char **out_error
) MATHSDK_NOEXCEPT;

MATHSDK_API void math_sdk_free_string(char *value) MATHSDK_NOEXCEPT;

#ifdef __cplusplus
}
#endif

#undef MATHSDK_NOEXCEPT

#endif
