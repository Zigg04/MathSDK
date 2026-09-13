# MathSDK

MathSDK is a native, embeddable mathematical engine. SymEngine is the primary
symbolic core. Capabilities not provided by SymEngine are implemented as C++
modules inside this SDK, without a Python or SymPy runtime fallback.

## Architecture

```text
Any host application (Dart, Kotlin, Swift, C++, ...)
    |
    | FFI / direct linkage
    v
Public C ABI: native/include/mathsdk/math_sdk.h
    |
    v
C++ facade: native/src/math_sdk.cpp
    |
    +-- SymEngine (private symbolic core)
    +-- MathSDK C++ modules (added only when product scope needs them)
```

Only the C ABI is stable and public. SymEngine types, exceptions, allocation,
and headers never cross the SDK boundary. Each returned string is owned by the
caller and released with `math_sdk_free_string`.

The build produces one shared library on Android and desktop. On Apple
platforms it produces a static library meant to be linked into a host app and
opened through the platform's own dynamic-library APIs (e.g. Dart's
`DynamicLibrary.process()`).

## Current API

| Function | What it does |
| --- | --- |
| `math_sdk_version` | Returns the SDK's version string. |
| `math_sdk_evaluate` | Parses and canonicalizes an exact expression (arithmetic, fractions, powers, roots, constants, symbolic expressions). |
| `math_sdk_evaluate_steps` | Same arithmetic subset as `math_sdk_evaluate`, plus a JSON reduction trace of the steps taken. |
| `math_sdk_substitute` | Substitutes a symbol with a value (numeric or symbolic), then canonicalizes. |
| `math_sdk_evaluate_numeric` | Evaluates to a `double` (no arbitrary precision — this build has no MPFR). |
| `math_sdk_derivative` | Derivative with respect to a symbol, any order. |
| `math_sdk_solve` | Single-variable equation solving; finite root sets only. |
| `math_sdk_solve_linear_system` | Linear systems, guarded against the singular/inconsistent case before solving. |
| `math_sdk_limit` | Limit of an expression at a point, one- or two-sided, via series expansion. |
| `math_sdk_integral` | Indefinite or definite integral over a fixed rule table (power rule, exp/log, trig, linearity, linear-argument substitution), verified by differentiating the result back. |
| `math_sdk_series` | Taylor/Laurent expansion of an expression around a point. |
| `math_sdk_matrix_op` | Determinant, inverse, transpose, addition, multiplication on dense matrices. |
| `math_sdk_ode_solve_separable` | Closed-form solution of a separable first-order ODE. |
| `math_sdk_free_string` | Releases a string returned by any of the above. |

Every function's exact coverage, ceiling, and error behavior is documented as
a comment directly above its declaration in
`native/include/mathsdk/math_sdk.h` — read that comment before assuming a
function covers your case. `CHANGELOG.md` records what each release added and
what it deliberately left out.

## Design constraints

- SymEngine `v0.14.0` and Boost `1.80.0` remain pinned.
- `boostmp` is the only integer backend; GMP is not part of the runtime.
- SymEngine is built thread-safe and linked statically into MathSDK.
- The exported ABI is C-compatible and exception-free.
- New mathematical modules stay private C++ code until a product operation
  requires a new C function.
- Android and iOS contain no Python or SymPy fallback.

## Evolution rules

- Public functions represent product operations such as `evaluate` or `solve`;
  the API does not mirror SymEngine's class hierarchy.
- The C++ facade owns input validation, exception translation, and output
  allocation. Mathematical modules do not know about any particular host
  runtime or FFI mechanism.
- Modules remain stateless unless measurements show that a persistent native
  context is necessary.
- There is no internal plugin registry or generic command dispatcher. A module
  and a C function are added only for a confirmed calculator capability.
- ABI additions are backward compatible; existing status values and function
  signatures are never repurposed.

## Build

Install the pinned dependencies as described in `validation/README.md`, then
configure the integrated SDK from the repository root.

Windows x64:

```powershell
$cmake = "$env:LOCALAPPDATA\Android\Sdk\cmake\3.22.1\bin\cmake.exe"
& $cmake -S MathSDK -B MathSDK\.build\sdk\windows-x64 `
  -G 'Visual Studio 17 2022' -A x64
& $cmake --build MathSDK\.build\sdk\windows-x64 `
  --config Release --target mathsdk_test --parallel
& "$env:LOCALAPPDATA\Android\Sdk\cmake\3.22.1\bin\ctest.exe" `
  --test-dir MathSDK\.build\sdk\windows-x64 `
  -C Release --output-on-failure
```

Android, repeated for `x86_64` and `arm64-v8a`:

```powershell
$sdk = "$env:LOCALAPPDATA\Android\Sdk"
$cmake = "$sdk\cmake\3.22.1\bin\cmake.exe"
$ndk = "$sdk\ndk\28.2.13676358"
$abi = 'arm64-v8a'
& $cmake -S MathSDK -B "MathSDK\.build\sdk\android-$abi" -G Ninja `
  -DCMAKE_TOOLCHAIN_FILE="$ndk\build\cmake\android.toolchain.cmake" `
  -DANDROID_ABI=$abi -DANDROID_PLATFORM=android-24 `
  -DANDROID_STL=c++_static -DCMAKE_BUILD_TYPE=Release
& $cmake --build "MathSDK\.build\sdk\android-$abi" `
  --target mathsdk_test --parallel
```

## Language bindings

MathSDK exposes nothing beyond the C ABI in `native/include/mathsdk/math_sdk.h`
— any host capable of calling a C function can consume it without
modification. This repository maintains one official binding:

- **Flutter/Dart** — [`flutter_package/`](flutter_package/), a federated
  Flutter plugin (`ffiPlugin: true`). Android compiles the native library
  from source via Gradle's `externalNativeBuild` + CMake; no prebuilt binary
  is bundled with the package. See its own README for usage and the current
  platform support matrix.

To write a binding for another language or framework (JNI/Kotlin, Swift via a
bridging header, direct C++ linkage, ...), see
[`docs/CONSUMERS.md`](docs/CONSUMERS.md) for the conventions any consumer
should follow — ownership rules, status-code translation, and what not to
depend on.

## Verified targets

| Target | Native library | Consumed end-to-end |
| --- | --- | --- |
| Windows x64 | Pass | Pass (native test suite) |
| Android `arm64-v8a` | Pass | Pass (via `flutter_package`, packaged into a running app) |
| Android `x86_64` | Pass | Pass (via `flutter_package`, packaged into a running app) |
| iOS device/simulator | Pending macOS/Xcode | Pending |

The Android library exports only the `math_sdk_*` functions declared in the
public header and depends only on `libm.so`, `libdl.so`, and `libc.so`.

The cross-platform toolchain findings and dependency setup are in
`validation/README.md`.

## License

MIT — see `LICENSE`. MathSDK statically links SymEngine (MIT) and Boost
(Boost Software License 1.0); their notices are reproduced in
`THIRD_PARTY_NOTICES.md`.

See `CHANGELOG.md` for release history.
