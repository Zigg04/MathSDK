#ifndef MATHSDK_SYMBOLIC_INTEGRAL_H
#define MATHSDK_SYMBOLIC_INTEGRAL_H

#include <symengine/basic.h>
#include <symengine/symbol.h>

namespace mathsdk
{

SymEngine::RCP<const SymEngine::Basic> try_integrate(
    const SymEngine::RCP<const SymEngine::Basic> &expr,
    const SymEngine::RCP<const SymEngine::Symbol> &variable
);

} // namespace mathsdk

#endif
