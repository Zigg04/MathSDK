# Changelog

## 0.2.0

- **Fixed: the package could not build when installed from pub.dev.** The
  Android build referenced native sources above the package directory, which
  the published archive does not contain. Gradle now downloads a prebuilt
  `libmathsdk.so` per ABI from the matching GitHub release, and still builds
  from source for `path:` dependencies on a repository checkout.
- Added `armeabi-v7a` alongside `arm64-v8a` and `x86_64`. Only the ABIs
  your build actually targets are downloaded.
- `MathSdk.open()` is now a per-isolate singleton instead of loading the
  library and resolving symbols again on every call.
- Added the `MathSdkAsync` extension: `evaluateAsync`, `derivativeAsync`,
  `solveAsync`, `integralAsync`, `seriesAsync`, and `matrixOpAsync` run on a
  background isolate so heavy symbolic work does not block the UI isolate.
- Added typed accessors so callers stop hand-parsing the native output:
  `solveRoots`, `solveSystem`, `matrix` (returning `MathMatrix`), and
  `evaluateWithSteps` (returning `MathStep`s). The raw-string methods are
  unchanged.
- Documented the `order` limits on `derivative` (1-10) and `series` (1-20),
  which the native library enforces.
- Documented the measured cost of each operation on `MathSdkAsync`, so the
  choice between the synchronous and background-isolate variants is based on
  numbers rather than guesswork.
- Prebuilt libraries are verified against the `SHA256SUMS` published with the
  release. A truncated or altered download now fails the build with a clear
  message instead of being packaged into the APK and failing at runtime.
- Prebuilt libraries are cached under the Gradle home instead of `build/`, so
  `flutter clean` no longer forces a re-download on every build.
- Fixed `MathSdk.version` reporting `0.1.0`. The version now comes from
  `pubspec.yaml` alone, instead of being repeated in the native CMake build
  and the Android Gradle build.
- A native call that reports success without writing a result now throws
  `MathSdkException` instead of dereferencing a null pointer.
- Documented `MathSdkStatus` and each of its values, plus instance lifetime
  and isolate behaviour.

## 0.1.1

Added dartdoc comments to the public API and shortened the package
description. No functional changes.

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
