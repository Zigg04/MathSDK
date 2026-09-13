# Changelog

All notable changes to MathSDK are documented in this file.

## [Unreleased]

### Added

- `math_sdk_evaluate_steps`: reduction trace for the same arithmetic subset
  `math_sdk_evaluate` covers, as a JSON array of `{before, after, rule}`
  steps. Cross-checked against `math_sdk_evaluate`'s own result before
  returning; disagreement is `MATHSDK_STATUS_INTERNAL_ERROR`, never a
  silently wrong trace.
- `math_sdk_substitute`: substitutes a symbol with a value (numeric or
  symbolic) in an expression, then canonicalizes.
- `math_sdk_evaluate_numeric`: evaluates an expression to a `double`. No
  arbitrary-precision path — SymEngine is built without MPFR
  (`HAVE_SYMENGINE_MPFR` undefined), so this is `double` precision only.
- `math_sdk_derivative`: derivative with respect to a symbol, any order
  (repeated `SymEngine::diff`), order clamped to `[1, 10]`.
- `math_sdk_solve`: single-variable equation solving (`lhs = rhs` or a bare
  zero-form expression). Returns a JSON array of roots for a finite solution
  set, `"[]"` for no solution. Infinite, symbolic, or otherwise non-finite
  solution sets (trigonometric equations, condition sets, etc.) return
  `MATHSDK_STATUS_NOT_IMPLEMENTED` rather than a lossy partial answer.
- `math_sdk_solve_linear_system`: linear systems (`;`-separated equations,
  `,`-separated variable names). Guarded by a determinant check on the
  coefficient matrix before solving — SymEngine's own systems-solving code
  does not handle inconsistent or underdetermined systems safely (an
  unguarded call on a singular system crashes); singular systems now return
  `MATHSDK_STATUS_NOT_IMPLEMENTED` instead.
- `math_sdk_limit`: one- or two-sided limit of an expression at a point, via
  series expansion (substitute `x = target + h`, expand in `h` around zero,
  read the lowest-order nonzero term) rather than L'Hopital — this also
  covers indeterminate forms without iterating a derivative that might never
  converge. Covers functions with a finite-order Taylor/Laurent series at the
  target; mixed asymptotic scales and non-analytic functions are
  `MATHSDK_STATUS_NOT_IMPLEMENTED`.
- `math_sdk_integral`: indefinite and definite integration over a fixed rule
  table (power rule, exp/log, sin/cos, `1/x`, `1/(1+x^2)`), linearity, and
  substitution for a linear inner argument. Every candidate antiderivative is
  verified by differentiating it back and comparing against the original
  integrand — a gap in the rule table is `MATHSDK_STATUS_NOT_IMPLEMENTED`,
  never a wrong answer. Definite integrals check for a singularity inside
  the interval (including its endpoints) before evaluating via the
  fundamental theorem of calculus.
- `math_sdk_series`: Taylor/Laurent expansion of an expression around a
  point, up to a given order, using the same series-expansion machinery as
  `math_sdk_limit`.
- `math_sdk_matrix_op`: determinant, inverse, transpose, addition, and
  multiplication on dense matrices (serialized as a flat
  `"rows;cols;v0,...,vN"` string — no opaque matrix type crosses the ABI).
  Inverse is guarded by a determinant check before calling into SymEngine,
  for the same reason as `math_sdk_solve_linear_system`.
- `math_sdk_ode_solve_separable`: closed-form solution of a first-order ODE
  already factored by the caller as `dy/dx = f(x) * g(y)`. Internally
  integrates `1/g(y)` and `f(x)` separately using the same rule-based
  integrator behind `math_sdk_integral`, so it inherits that integrator's
  exact coverage and verification. Linear, homogeneous, and exact first-order
  ODEs are out of scope for this version — they need integration techniques
  (by parts, partial fractions) or a variable-substitution engine this SDK
  does not implement yet.

### Fixed

- A parallel (multi-core) build under `CMAKE_BUILD_TYPE=Debug` could fail
  with `Teuchos_config.h file not found`. SymEngine defaults to
  `WITH_SYMENGINE_RCP=OFF` in Debug builds, which pulls in a vendored
  Teuchos::RCP implementation; that vendored code links against Teuchos
  without declaring a build-order dependency on its own generated
  configuration header, so a sufficiently parallel build could compile
  sources that need the header before it existed. Fixed by forcing
  `WITH_SYMENGINE_RCP=ON` in every build configuration, which avoids Teuchos
  entirely.

### Known limitations

- `math_sdk_evaluate_numeric` has no precision knob (no MPFR in this build).
- `math_sdk_solve` only returns finite root sets; trigonometric and other
  symbolic solution forms are out of scope until a richer output contract is
  designed.
- `math_sdk_integral` and `math_sdk_ode_solve_separable` cover a fixed rule
  table, not general symbolic integration (no integration by parts, no
  partial fractions, no trigonometric substitution).
- `math_sdk_ode_solve_separable` only covers the separable case; linear,
  homogeneous, and exact first-order ODEs are not supported.

## [0.1.0] - 2026-09-05

Initial release.

### Added

- Public C ABI (`native/include/mathsdk/math_sdk.h`): `math_sdk_version`,
  `math_sdk_evaluate`, `math_sdk_free_string`.
- `math_sdk_evaluate` parses and canonicalizes exact expressions covering
  arithmetic, fractions, powers, roots, constants, and symbolic expressions
  via a pinned SymEngine `v0.14.0` core (`INTEGER_CLASS=boostmp`, Boost
  `1.80.0`, no GMP).
- Typed error reporting through `MathSdkStatus` (invalid argument, parse
  error, division by zero, domain error, not implemented, internal error) —
  no exception ever crosses the ABI boundary.
- CMake build producing a shared library on Android/Windows and a static
  library on Apple platforms, with hidden SymEngine symbol visibility.
- Native test suite (`native/tests/math_sdk_test.cpp`) covering success,
  parse errors, division by zero, domain errors, and null-pointer misuse.
- Cross-platform toolchain validation for Windows x64 and Android
  (`arm64-v8a`, `x86_64`); iOS validation pending macOS/Xcode access.

### Known limitations

- No `solve` or other product operation beyond `evaluate` yet — added only
  when a confirmed calculator capability needs one.
- iOS has not been built or executed; treat it as unsupported until
  `validation/README.md`'s iOS gate passes.
- No continuous integration yet; verified targets are validated manually per
  the table in `README.md`.
