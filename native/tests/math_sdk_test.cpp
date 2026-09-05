#include "mathsdk/math_sdk.h"

#include <cstring>
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

    if (failures == 0) {
        std::cout << "MathSDK native tests passed\n";
    }
    return failures == 0 ? 0 : 1;
}
