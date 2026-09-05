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

- `math_sdk_version`
- `math_sdk_evaluate`
- `math_sdk_free_string`

`math_sdk_evaluate` parses and canonicalizes exact expressions. It supports
the arithmetic, fractions, powers, roots, constants, and symbolic expressions
already handled by the pinned SymEngine core.

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

## Using MathSDK from a host application

MathSDK exposes nothing beyond the C ABI in `native/include/mathsdk/math_sdk.h`
— any host capable of calling a C function (Dart FFI, JNI/Kotlin, Swift via a
bridging header, or direct C++ linkage) can consume it without modification.
See `docs/CONSUMERS.md` for a worked example (the Flutter/Dart binding) and
the packaging conventions a new consumer should follow.

## Verified targets

| Target | Integrated SDK | Native execution |
| --- | --- | --- |
| Windows x64 | Pass | Pass |
| Android `x86_64` | Pass | APK packaging pass; device run pending |
| Android `arm64-v8a` | Pass | APK packaging pass; device run pending |
| iOS device/simulator | Pending macOS/Xcode | Pending |

The Android libraries export only the three `math_sdk_*` functions and depend
only on `libm.so`, `libdl.so`, and `libc.so`. After stripping debug symbols,
the current library is about 1.7 MB for `x86_64` and 1.8 MB for `arm64-v8a`.

The cross-platform toolchain findings and dependency setup are in
`validation/README.md`.

## License

MIT — see `LICENSE`. MathSDK statically links SymEngine (MIT) and Boost
(Boost Software License 1.0); their notices are reproduced in
`THIRD_PARTY_NOTICES.md`.

See `CHANGELOG.md` for release history.
