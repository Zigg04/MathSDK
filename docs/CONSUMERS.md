# Consuming MathSDK

MathSDK's only public surface is the C ABI in
`native/include/mathsdk/math_sdk.h`. Any host application links against the
built library (or loads it dynamically) and calls the three exported
functions directly — there is no required SDK-side wrapper for a given
language or framework. This document shows one worked integration and the
conventions a new consumer should follow.

## Conventions for any consumer

- Load the shared library (`libmathsdk.so` / `mathsdk.dll`) or link the static
  library (Apple platforms) built from this repository.
- Call `math_sdk_evaluate` with a null-terminated expression string and two
  output pointers.
- Always release the returned result/error strings with
  `math_sdk_free_string`, even on the error path.
- Translate the returned `MathSdkStatus` into whatever error type is
  idiomatic on the host side (exception, `Result`, sealed class, ...). MathSDK
  itself never throws across the ABI boundary.
- Do not depend on anything outside `math_sdk.h` — internal SymEngine types,
  headers, or build artifacts are not part of the contract and can change
  between versions.

## Worked example: Flutter/Dart via `dart:ffi`

The Bernify app consumes MathSDK this way. The binding lives in the
consumer's own repository at
`bernify_app/lib/features/math_engine/data/services/math_sdk_service.dart`.
It loads `libmathsdk.so` on Android, `mathsdk.dll` on Windows, and the current
process on Apple platforms. Native failures become a typed exception
(`MathSolverException`) on the Dart side, and every returned string is
released through `math_sdk_free_string`.

The stripped Android artifacts are packaged inside the consuming app at:

```text
bernify_app/android/app/src/main/jniLibs/arm64-v8a/libmathsdk.so
bernify_app/android/app/src/main/jniLibs/x86_64/libmathsdk.so
```

That app contains no Chaquopy, Python, SymPy, or math MethodChannel — all
symbolic evaluation goes through this SDK's C ABI. Rebuild and strip the SDK
artifacts before replacing those files after a native API or implementation
change.
