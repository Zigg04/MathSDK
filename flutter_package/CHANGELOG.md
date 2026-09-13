# Changelog

## 0.1.0

Initial release. Wraps MathSDK's native C ABI (native/include/mathsdk/math_sdk.h)
via `dart:ffi`:

- `evaluate`, `evaluateSteps` — exact expression evaluation, with a
  step-by-step reduction trace for the arithmetic subset.
- `substitute`, `evaluateNumeric` — variable substitution and numeric
  (double) evaluation.
- `derivative`, `integral`, `series`, `limit` — calculus operations, each
  documented in the native header with its exact coverage and ceiling.
- `solve`, `solveLinearSystem` — equation and linear system solving.
- `matrixOp` — determinant, inverse, transpose, add, multiply.
- `odeSolveSeparable` — separable first-order ODEs only.

Android only in this version (Gradle `externalNativeBuild` compiles the
native library from source — no prebuilt binaries are shipped or copied by
hand). iOS/desktop support is tracked separately; see the root MathSDK
repository's README for platform status.
