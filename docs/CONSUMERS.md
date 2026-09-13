# Consuming MathSDK

MathSDK's only public surface is the C ABI in
`native/include/mathsdk/math_sdk.h`. Any host application links against the
built library (or loads it dynamically) and calls the exported functions
directly — there is no required SDK-side wrapper for a given language or
framework. This document covers the conventions any consumer should follow,
and points to the official Flutter/Dart binding as a worked example.

## Conventions for any consumer

- Load the shared library (`libmathsdk.so` / `mathsdk.dll`) or link the static
  library (Apple platforms) built from this repository.
- Every function follows the same output-pointer convention: pass
  uninitialized `char**` output slots, check the returned `MathSdkStatus`,
  and only read the outputs when the status is
  `MATHSDK_STATUS_OK`.
- Always release every returned string with `math_sdk_free_string`, even on
  the error path — a non-OK status can still allocate an error message that
  the caller owns.
- Translate the returned `MathSdkStatus` into whatever error type is
  idiomatic on the host side (exception, `Result`, sealed class, ...). MathSDK
  itself never throws or crashes across the ABI boundary — a failure is
  always a typed status, not an exception.
- Treat `MATHSDK_STATUS_NOT_IMPLEMENTED` as "outside this version's supported
  rule set," not as a bug — most functions document a deliberately limited
  scope (see the comment above each function's declaration) rather than
  attempting general symbolic computation.
- Do not depend on anything outside `math_sdk.h` — internal SymEngine types,
  headers, or build artifacts are not part of the contract and can change
  between versions.

## Writing a new binding

A binding for a new language or framework needs to:

1. Declare the C ABI's function signatures in that language's FFI mechanism
   (or generate them from the header, as the Flutter binding does with
   `ffigen`).
2. Wrap each raw call with the output-pointer/status-check/free-string
   pattern above, so callers of the binding see an idiomatic
   result-or-exception API instead of raw pointers and status codes.
3. Load the platform-appropriate binary: a shared library on Android/Linux/
   Windows, or the current process on Apple platforms (where MathSDK builds
   as a static library linked into the host app).

## Worked example: Flutter/Dart via `dart:ffi`

The official Flutter/Dart binding lives in this repository at
[`flutter_package/`](../flutter_package/) — a federated Flutter plugin
(`ffiPlugin: true`) that follows exactly the pattern above:

- `lib/src/bindings_generated.dart` — raw FFI bindings generated from
  `math_sdk.h` via `ffigen`; never edited by hand, regenerated whenever the
  header changes.
- `lib/src/mathsdk_ffi.dart` — the idiomatic wrapper: allocates and frees
  native memory, and turns a non-OK `MathSdkStatus` into a thrown
  `MathSdkException` instead of a status code the caller has to check.
- `android/CMakeLists.txt` — includes this repository's root `CMakeLists.txt`
  so Gradle compiles the native library from source via
  `externalNativeBuild`; no prebuilt binary is bundled with the package.

See that package's own README for usage examples and its current platform
support matrix.
