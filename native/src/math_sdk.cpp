#include "mathsdk/math_sdk.h"

#include <cctype>
#include <cstdlib>
#include <cstring>
#include <exception>
#include <string>

#include <symengine/constants.h>
#include <symengine/parser.h>
#include <symengine/symengine_exception.h>

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
        const auto result = parsed->__str__();
        *out_result = copy_string(result.c_str());
        if (*out_result == nullptr) {
            return write_error(
                MATHSDK_STATUS_INTERNAL_ERROR,
                "Unable to allocate result",
                out_error
            );
        }
        return MATHSDK_STATUS_OK;
    } catch (SymEngine::SymEngineException &error) {
        return write_error(
            map_symengine_status(error.error_code()),
            error.what(),
            out_error
        );
    } catch (const std::exception &error) {
        return write_error(
            MATHSDK_STATUS_INTERNAL_ERROR,
            error.what(),
            out_error
        );
    } catch (...) {
        return write_error(
            MATHSDK_STATUS_INTERNAL_ERROR,
            "Unknown native error",
            out_error
        );
    }
}

void math_sdk_free_string(char *value) noexcept
{
    std::free(value);
}
