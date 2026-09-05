#include <cstring>

#include <symengine/cwrapper.h>

extern "C" int mathsdk_toolchain_probe()
{
    basic_struct *expression = basic_new_heap();

    const auto status = basic_parse(expression, "2 + 3 * 4");
    if (status != SYMENGINE_NO_EXCEPTION) {
        basic_free_heap(expression);
        return status;
    }

    char *result = basic_str(expression);
    const bool is_expected = std::strcmp(result, "14") == 0;
    basic_str_free(result);
    basic_free_heap(expression);
    return is_expected ? 0 : -1;
}
