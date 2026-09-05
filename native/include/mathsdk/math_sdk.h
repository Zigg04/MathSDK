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

typedef enum MathSdkStatus {
    MATHSDK_STATUS_OK = 0,
    MATHSDK_STATUS_INVALID_ARGUMENT = 1,
    MATHSDK_STATUS_PARSE_ERROR = 2,
    MATHSDK_STATUS_DIVISION_BY_ZERO = 3,
    MATHSDK_STATUS_DOMAIN_ERROR = 4,
    MATHSDK_STATUS_NOT_IMPLEMENTED = 5,
    MATHSDK_STATUS_INTERNAL_ERROR = 100
} MathSdkStatus;

MATHSDK_API const char *math_sdk_version(void) MATHSDK_NOEXCEPT;

/* Both output pointers are required. Release returned strings with
 * math_sdk_free_string. */
MATHSDK_API MathSdkStatus math_sdk_evaluate(
    const char *expression,
    char **out_result,
    char **out_error
) MATHSDK_NOEXCEPT;

MATHSDK_API void math_sdk_free_string(char *value) MATHSDK_NOEXCEPT;

#ifdef __cplusplus
}
#endif

#undef MATHSDK_NOEXCEPT

#endif
