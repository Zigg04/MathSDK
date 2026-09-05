# Changelog

All notable changes to MathSDK are documented in this file.

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
