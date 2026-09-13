#ifndef MATHSDK_ARITHMETIC_STEPS_H
#define MATHSDK_ARITHMETIC_STEPS_H

#include <stdexcept>
#include <string>

namespace mathsdk
{


class ArithmeticStepsError : public std::runtime_error
{
public:
    explicit ArithmeticStepsError(const std::string &message)
        : std::runtime_error(message)
    {
    }
};


std::string arithmetic_steps_to_json(
    const std::string &expression,
    std::string &out_final_result
);

} // namespace mathsdk

#endif
